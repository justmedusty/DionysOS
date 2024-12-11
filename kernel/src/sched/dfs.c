//
// Created by dustyn on 9/14/24.
//

/*
	Dustyn's fair scheduler
 */
#define _DFS_ // this is only here because clion is complaining and I lose intellisense because reading a makefile is too difficult and complicated. This is meant to just be defined in the makefile

#ifdef _DFS_
#include <include/arch/arch_asm_functions.h>
#include <include/arch/arch_timer.h>
#include <include/data_structures/doubly_linked_list.h>
#include <include/data_structures/singly_linked_list.h>
#include "include/scheduling/sched.h"
#include <include/definitions.h>
#include <include/data_structures/spinlock.h>
#include "include/data_structures/queue.h"
#include "include/drivers/serial/uart.h"
#include "include/arch/arch_smp.h"
#include "include/arch/arch_cpu.h"
#include "include/definitions/string.h"
#include <include/mem/kalloc.h>
#include <include/mem/mem.h>


struct queue sched_global_queue; // this is where processes will be stuck for CPUs to poach when they aren't busy
struct queue global_sleep_queue;

struct spinlock sched_global_lock;
struct spinlock sched_sleep_lock;
struct spinlock purge_lock;
struct singly_linked_list dead_processes;

static void purge_dead_processes();
static void look_for_process();
extern void context_switch(struct gpr_state *old,struct gpr_state *new);

static void free_process(struct process *process) {
  process->current_working_dir->vnode_active_references--;

  kfree((void *)process->current_gpr_state->rsp - DEFAULT_STACK_SIZE);

  kfree(process->current_gpr_state);
  doubly_linked_list_destroy(process->handle_list->handle_list);

  kfree(process->handle_list);

  if (process->page_map != kernel_pg_map) {
    arch_dealloc_page_table(process->page_map->top_level);
    kfree(V2P(process->page_map->top_level));
    kfree(process->page_map);
  }
}

void sched_init() {
  singly_linked_list_init(&dead_processes,0);
  initlock(&sched_global_lock, sched_LOCK);
  initlock(&purge_lock, sched_LOCK);
  initlock(&sched_sleep_lock, sched_LOCK);
  queue_init(&sched_global_queue,QUEUE_MODE_FIFO,"sched_global");
  queue_init(&global_sleep_queue,QUEUE_MODE_FIFO,"global_sleep");
  for(uint32_t i = 0; i < cpu_count; i++) {
    queue_init(&local_run_queues[i],QUEUE_MODE_FIFO,"dfs");
    cpu_list[i].local_run_queue = &local_run_queues[i];
  }
  serial_printf("DFS: Local CPU RQs Initialized \n");
}

void scheduler_main(void) {
  for (;;) {
    sched_run();
  }

}



void sched_yield() {
  struct process *process = my_cpu()->running_process;
  process->current_state = PROCESS_READY;
  enqueue(my_cpu()->local_run_queue,process,process->priority);
  context_switch(my_cpu()->running_process->current_gpr_state,my_cpu()->scheduler_state);
}

void sched_run() {
  struct cpu *cpu = my_cpu();

  if (cpu->local_run_queue->head == NULL) {
    timer_sleep(1000);
    serial_printf("DFS: Local Run Queue is Empty \n");
    purge_dead_processes(); /* This doesn't allow for an explicit wait maybe I will change that later*/
    look_for_process();
    return;
  }

  cpu->running_process = cpu->local_run_queue->head->data;
  cpu->running_process->current_state = PROCESS_RUNNING;
  dequeue(cpu->local_run_queue);
  context_switch(cpu->scheduler_state,cpu->running_process->current_gpr_state);
}


void sched_preempt() {
  struct cpu* cpu = my_cpu();
  struct process *process = cpu->running_process;
  enqueue(my_cpu()->local_run_queue,process,process->priority);
  process->current_state = PROCESS_READY;
  context_switch(my_cpu()->running_process->current_gpr_state,cpu->scheduler_state);
}


void sched_sleep(void *sleep_channel) {
  struct cpu* cpu = my_cpu();
  struct process *process = cpu->running_process;

}

void sched_claim_process() {

}

void sched_exit() {
  struct cpu *cpu = my_cpu();
  struct process *process = cpu->running_process;
  singly_linked_list_insert_head(&dead_processes,process);
  my_cpu()->running_process = NULL;
  context_switch(process->current_gpr_state,my_cpu()->scheduler_state);
}


static void purge_dead_processes() {
  if (dead_processes.node_count == 0) {
    return;
  }
  acquire_spinlock(&purge_lock);
  const struct singly_linked_list_node *node = dead_processes.head;

  while (node != NULL) {
    free_process(node->data);
    singly_linked_list_remove_head(&dead_processes);
    node = dead_processes.head;
  }
  release_spinlock(&purge_lock);
}

static void look_for_process() {
  struct cpu *cpu = my_cpu();
  acquire_spinlock(&sched_global_lock);

  if (sched_global_queue.queue_mode == 0) {
    release_spinlock(&sched_global_lock);
    return;
  }
  struct process *process = sched_global_queue.head->data;
  dequeue(&sched_global_queue);
  enqueue(cpu->local_run_queue,process,process->priority);
  release_spinlock(&sched_global_lock);
  return;
}

#endif