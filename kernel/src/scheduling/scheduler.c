//
// Created by dustyn on 9/14/24.
//

/*
	Dustyn's fair scheduler
 */
#include <include/drivers/display/framebuffer.h>

#include <include/architecture/arch_asm_functions.h>
#include <include/architecture/arch_timer.h>
#include <include/data_structures/doubly_linked_list.h>
#include <include/data_structures/singly_linked_list.h>
#include "include/scheduling/sched.h"
#include <include/definitions/definitions.h>
#include <include/data_structures/spinlock.h>
#include "include/data_structures/queue.h"
#include "include/drivers/serial/uart.h"
#include "include/architecture/arch_smp.h"
#include "include/architecture/arch_cpu.h"
#include "include/definitions/string.h"
#include <include/memory/kalloc.h>
#include <include/memory/mem.h>


struct queue sched_global_queue; // this is where processes will be stuck for CPUs to poach when they aren't busy
struct doubly_linked_list global_sleep_queue;

struct spinlock sched_global_lock;
struct spinlock sched_sleep_lock;
struct spinlock purge_lock[MAX_CPUS];
struct singly_linked_list dead_processes[MAX_CPUS];

static void purge_dead_processes();

static void look_for_process();

extern void context_switch(struct register_state *old, struct register_state *new);

/*
 * This function frees all the internal datastructures and does other housekeeping for after a process has
 * terminated and we need to remove its struct from memory
 */
static void free_process(struct process *process) {
    process->current_working_dir->vnode_active_references--;

    kfree(process->stack);

    kfree(process->current_register_state);
    doubly_linked_list_destroy(process->handle_list->handle_list, true);

    kfree(process->handle_list);

    if (process->page_map->top_level != kernel_pg_map->top_level) {
        arch_dealloc_page_table(process->page_map->top_level);
        kfree(P2V(process->page_map->top_level)); // we do this because kfree is locked, phys_dealloc is not
    }

    doubly_linked_list_destroy(process->page_map->vm_regions, true);
    kfree(process->page_map->vm_regions);
    kfree(process->page_map);

}

/*
 * Simple init function that sets up data structures for us
 */
void sched_init() {
    kprintf("Initializing Scheduler...\n");

    for(size_t i = 0; i <= cpus_online; i++ ){
        singly_linked_list_init(&dead_processes[i], 0);
    }

    initlock(&sched_global_lock, sched_LOCK);

    for(size_t i = 0; i <= cpus_online; i++ ){
        initlock(&purge_lock[i], sched_LOCK);
    }

    initlock(&sched_sleep_lock, sched_LOCK);
    doubly_linked_list_init(&global_sleep_queue);
    for (uint32_t i = 0; i < cpu_count; i++) {

#ifdef _DFS_
        queue_init(&local_run_queues[i], QUEUE_MODE_FIFO, "dfs");
#endif

#ifdef _DPS_
        queue_init(&local_run_queues[i], QUEUE_MODE_PRIORITY, "dps");
#endif

        cpu_list[i].local_run_queue = &local_run_queues[i];
    }

#ifdef _DFS_
    queue_init(&sched_global_queue, QUEUE_MODE_FIFO, "sched_global");
#endif

#ifdef _DPS_
    queue_init(&sched_global_queue, QUEUE_MODE_PRIORITY, "sched_global");
#endif

    kprintf("Scheduler initialized\n");
    serial_printf("DFS: Local CPU RQs Initialized \n");
}

/*
 * Infinite loop so when we jump back into the scheduler from a task exit or preempt, it will continually attempt to run tasks over and over again
 */
_Noreturn void scheduler_main(void) {
    for (;;) {
        sched_run();
    }
}

/*
 * Yield the scheduler , swap registers and jump back into the mouth of the scheduler
 */
void sched_yield() {
    struct process *process = my_cpu()->running_process;
    process->current_state = PROCESS_READY;
    enqueue(my_cpu()->local_run_queue, process, process->priority);
    context_switch(my_cpu()->running_process->current_register_state, my_cpu()->scheduler_state);
}

/*
 * Scheduler run function. If there is an active process waiting in the local run queue,
 * run it. Set the cpu running process and state and then jump into the process context.
 *
 * If there is nothing to run, purge dead processes from the dead process queue (this prevents any sort of posix wait() functionality but is fine for now)
 * It also will call look_for_process which will peruse the global run queue for any spare processes that are still homeless. Will take the poor process under its wing and
 * give it some sweet sweet cpu time.
 */
void sched_run() {
    struct cpu *cpu = my_cpu();
    if (cpu->local_run_queue->head == NULL) {

        timer_sleep(1500);

#ifdef _DFS_
        serial_printf("DFS: Local Run Queue is Empty \n");
#endif
#ifdef _DPS_
        serial_printf("DPS: Local Run Queue is Empty \n");
#endif
        purge_dead_processes(); /* This doesn't allow for an explicit wait maybe I will change that later*/
        look_for_process();
        current_pos_cursor(framebuffer_device.device_info);
        return;
    }

    cpu->running_process = cpu->local_run_queue->head->data;
    cpu->running_process->current_cpu = cpu;
    cpu->running_process->current_state = PROCESS_RUNNING;

    dequeue(cpu->local_run_queue);
    cpu->running_process->start_time = timer_get_current_count(); // will be used for timekeeping


    context_switch(cpu->scheduler_state, cpu->running_process->current_register_state);
}

/*
 * Preempt simply puts the running process in the queue again and then places it back into the queue and jumps back
 * into scheduler context
 */
void sched_preempt() {
    struct cpu *cpu = my_cpu();
    struct process *process = cpu->running_process;
    process->ticks_taken += process->start_time;
    process->start_time = 0;
    enqueue(my_cpu()->local_run_queue, process, process->priority);
    process->current_state = PROCESS_READY;
    context_switch(my_cpu()->running_process->current_register_state, cpu->scheduler_state);
}

/*
 * This is naive and should follow the Linux impl of checking a condition occasionally so
 * you don't end up with the classic sleep on condition but condition already changed before you went to sleep.
 *  This is fine for now but I'm commenting so it is visible both for others and for me later
 */
void sched_sleep(void *sleep_channel) {
    struct process *process = current_process();
    doubly_linked_list_insert_head(&global_sleep_queue, process);
    process->sleep_channel = sleep_channel;
    process->current_state = PROCESS_SLEEPING;
    process->ticks_taken += process->start_time;
    process->start_time = timer_get_current_count();
    context_switch(my_cpu()->running_process->current_register_state, process->current_cpu->scheduler_state);
}

void sched_wakeup(const void *wakeup_channel) {
    acquire_spinlock(&sched_sleep_lock);
    struct doubly_linked_list_node *node = global_sleep_queue.head;
    if (node == NULL) {
        return;
    }
    struct process *process = node->data;
    while (node && process) {
        process = node->data;

        if (process->sleep_channel == wakeup_channel) {
            process->sleep_channel = 0;
            process->ticks_slept += process->start_time;
            process->start_time = 0;
            doubly_linked_list_remove_node_by_address(&global_sleep_queue, node);
        }
        node = node->next;
    }
    release_spinlock(&sched_sleep_lock);
};

/*
 * Attempts to steal a process from a rival processor, only bother stealing if a run-queue is longer than 2 nodes
 */
void sched_claim_process() {
    struct cpu *this_cpu = my_cpu();
    for (size_t i = 0; i < cpus_online; i++) {

        if (cpu_list[i].cpu_id == this_cpu->cpu_id) {
            continue;
        }

        acquire_spinlock(cpu_list[i].local_run_queue->spinlock);

        struct queue *queue = cpu_list[i].local_run_queue;

        if (queue->node_count == 0) {
            goto done;
        }

        if (queue->node_count > 2) {

            enqueue(this_cpu->local_run_queue, queue->tail, queue->tail->priority);
            this_cpu->local_run_queue->tail = this_cpu->local_run_queue->tail->prev;
            this_cpu->local_run_queue->node_count--;
            goto done;

        }

        done:
        release_spinlock(cpu_list[i].local_run_queue->spinlock);

    }
}

/*
 * Exit  for when a process is finished execution. The process will be added to the dead list, and we jump back into scheduler context
 */
void sched_exit() {
    struct cpu *cpu = my_cpu();
    struct process *process = cpu->running_process;
    singly_linked_list_insert_head(&dead_processes[cpu->cpu_id], process);
    my_cpu()->running_process = NULL;
    context_switch(process->current_register_state, cpu->scheduler_state);
}

/*
 * Purge dead processes from the dead list, this will not be kept if I decide to add the posix wait() functionality
 */
static void purge_dead_processes() {
    struct cpu *current_cpu = my_cpu();
    if (dead_processes[current_cpu->cpu_id].node_count == 0) {
        return;
    }
    acquire_spinlock(&purge_lock[current_cpu->cpu_id]);
    const struct singly_linked_list_node *node = dead_processes[current_cpu->cpu_id].head;
    while (node != NULL) {
        struct process *p = node->data;

        free_process(node->data);
        singly_linked_list_remove_head(&dead_processes[current_cpu->cpu_id]);
        node = dead_processes[current_cpu->cpu_id].head;

    }
    release_spinlock(&purge_lock[current_cpu->cpu_id]);
}

/*
 * Attempt to grab a process from the global queue for the local CPU
 * queue
 */
static void look_for_process() {
    struct cpu *cpu = my_cpu();
    acquire_spinlock(&sched_global_lock);

    if (sched_global_queue.node_count == 0) {
        release_spinlock(&sched_global_lock);
        return;
    }

    struct process *process = sched_global_queue.head->data;
    dequeue(&sched_global_queue);
    enqueue(cpu->local_run_queue, process, process->priority);
    release_spinlock(&sched_global_lock);
}

//#ifdef _DPS_
static void promote_processes() {
    struct cpu *cpu = my_cpu();
    struct queue *local_runqueue = cpu->local_run_queue;
    acquire_spinlock(local_runqueue->spinlock);
    struct queue_node *current = local_runqueue->head;

    if (current == NULL) {
        release_spinlock(local_runqueue->spinlock);
        return;
    }

}
//#endif