//
// Created by dustyn on 12/7/24.
//
#include <stdint.h>
#include "include/scheduling/kthread.h"

#include "include/definitions/string.h"
#include <include/architecture/arch_asm_functions.h>
#include <include/architecture/arch_timer.h>
#include <include/data_structures/doubly_linked_list.h>
#include <include/drivers/display/framebuffer.h>
#include <include/filesystem/vfs.h>
#include <include/scheduling/sched.h>
#include "include/definitions/definitions.h"
#include "include/memory/kmalloc.h"
#include "include/scheduling/process.h"

static uint64_t kthread_pid = 50000;
/*
 * Initialize a kthread and add it to the local run-queue
 */

static uint64_t get_kthread_pid(){
    //Just reserve 50k+ for kthread pids
    kthread_pid++;
    if(kthread_pid == 0){
        kthread_pid = 50000;
    }
    return kthread_pid;
}
void kthread_init() {
    struct process *proc = kzmalloc(sizeof(struct process));

    proc->current_cpu = my_cpu();

    proc->current_working_dir = &vfs_root;

    vfs_root.vnode_active_references++;

    proc->handle_list = kzmalloc(sizeof(struct virtual_handle_list));

    proc->handle_list->handle_list = kzmalloc(sizeof(struct doubly_linked_list));

    doubly_linked_list_init(proc->handle_list->handle_list);


    struct virt_map *kthread_map = kzmalloc(sizeof(struct virt_map));
    kthread_map->top_level = kernel_pg_map->top_level;
    proc->page_map = kthread_map;

    proc->parent_process_id = 0;
    proc->process_type = KERNEL_THREAD;
    proc->current_register_state = kzmalloc(sizeof(struct register_state));
    proc->process_id = get_kthread_pid();
    memset(proc->current_register_state, 0, sizeof(struct register_state));

    /*
     * Create dedicated region, share kernel page table but have their own region for stack
     */
    void *stack = kzmalloc(DEFAULT_STACK_SIZE);
    /*
     * A lot of the flags here are not actually needed since there is no new mappings
     */
    struct virtual_region *stack_region = create_region((uint64_t)
    stack, DEFAULT_STACK_SIZE / PAGE_SIZE, STACK, READWRITE, true);
    attach_region(proc->page_map, stack_region);
    proc->stack = stack;

    /*
     * Set the architecture specific registers ahead of time the stack is set up as well as the instruction pointer
     * The instruction pointer points to kthread main
     */
#ifdef __x86_64__
    proc->current_register_state->rip = (uint64_t) kthread_main; // it's grabbing a junk value if not called from an interrupt so overwriting rip with kthread main
    proc->current_register_state->rsp = (uintptr_t)(proc->stack )+ DEFAULT_STACK_SIZE; /* Allocate a private stack */
    proc->current_register_state->rbp = proc->current_register_state->rsp - 8; /* Set base pointer to the new stack pointer, -8 for return address */
    proc->kernel_stack = stack;
#endif

    proc->current_register_state->interrupts_enabled = are_interrupts_enabled();

    if (proc->current_cpu->local_run_queue == NULL) {
        proc->current_cpu->local_run_queue = &local_run_queues[proc->current_cpu->cpu_number];
    }

    enqueue(proc->current_cpu->local_run_queue, proc, MEDIUM);
    kprintf("Kernel Threads Initialized For CPU #%i\n", my_cpu()->cpu_number);
}

/*
 * For the time being, this function can't return
 */
void kthread_main() {
    uint8_t cpu_no = my_cpu()->cpu_number;
    serial_printf("kthread active on cpu %i\n", cpu_no);
    serial_printf("Timer ticks %i\n", timer_get_current_count());
    char *buffer = kmalloc(PAGE_SIZE);
    kthread_work(NULL,NULL);
    int64_t handle = open("/temp/procfs/kernel_messages");
    if (handle < 0) {
        goto done;
    }

    DEBUG_PRINT("READING ON KTHREAD %i\n",current_process()->process_id);
    int64_t ret = read(handle,buffer, 0);

    if(ret < 0){
        warn_printf("kthread_main: Could not read file!");
        sched_exit();
    }

    DEBUG_PRINT("KERNELMSG FILE: %s\n",buffer);
    done:
    sched_yield();
    serial_printf("Thread %i back online\n", cpu_no);
    timer_sleep(10);
    serial_printf("Thread %i exiting\n", cpu_no);
    seek(handle, SEEK_END);
    close(handle);

    kfree(buffer);
    sched_exit();
}

void kthread_work(worker_function function, void *args) {

    char *message_buffer = kmalloc(PAGE_SIZE);
    ksprintf(message_buffer, "Kernel thread %i is starting, calling function located at %x.64\n",
             current_process()->process_id, function);
    log_kernel_message(message_buffer);

    if (function) {
        function(args);
    } else{
        ksprintf(message_buffer, "Kernel thread %i was called with a NULL function pointer!\n");
    }

    ksprintf(message_buffer, "Kernel thread %i is exiting, called function located at %x.64\n",current_process()->process_id, function);
    kfree(message_buffer);
    return;
    sched_exit();
}

