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

#include "include/memory/kalloc.h"
#include "include/scheduling/process.h"

/*
 * Initialize a kthread and add it to the local run-queue
 */
void kthread_init() {
    struct process *proc = kmalloc(sizeof(struct process));
    memset(proc, 0, sizeof(struct process));
    proc->current_cpu = my_cpu();
    proc->current_working_dir = &vfs_root;
    vfs_root.vnode_active_references++;
    proc->handle_list = kmalloc(sizeof(struct virtual_handle_list));
    proc->handle_list->handle_list = kmalloc(sizeof(struct doubly_linked_list));
    doubly_linked_list_init(proc->handle_list->handle_list);
    proc->handle_list->handle_id_bitmap = 0;
    proc->handle_list->num_handles = 0;
    proc->page_map = kernel_pg_map;
    proc->parent_process_id = 0;
    proc->process_type = KERNEL_THREAD;
    proc->current_register_state = kmalloc(sizeof(struct register_state));
    memset(proc->current_register_state, 0, sizeof(struct register_state));
    proc->stack = kmalloc(DEFAULT_STACK_SIZE);

#ifdef __x86_64__
    proc->current_register_state->rip = (uint64_t) kthread_main; // it's grabbing a junk value if not called from an interrupt so overwriting rip with kthread main
    proc->current_register_state->rsp = (uintptr_t)(proc->stack )+ DEFAULT_STACK_SIZE; /* Allocate a private stack */
    proc->current_register_state->rbp = proc->current_register_state->rsp - 8; /* Set base pointer to the new stack pointer, -8 for return address*/
#endif

    proc->current_register_state->interrupts_enabled = are_interrupts_enabled();

    if (proc->current_cpu->local_run_queue == NULL) {
        proc->current_cpu->local_run_queue = &local_run_queues[proc->current_cpu->cpu_number];
    }

    enqueue(proc->current_cpu->local_run_queue, proc, MEDIUM);
    kprintf("Kernel Threads Initialized For CPU #%i\n",my_cpu()->cpu_number);
}

/*
 * For the time being, this function can't return
 */
void kthread_main() {
    uint8_t cpu_no = my_cpu()->cpu_number;
    serial_printf("kthread active on cpu %i\n", cpu_no);
    serial_printf("Timer ticks %i\n", timer_get_current_count());
    char *buffer = kmalloc(PAGE_SIZE * 8);
    strcpy(buffer,"kthread_main Using allocated buffer about to yield scheduler");
    serial_printf("%s \n", buffer);
    int64_t handle = open("/etc/passwd");
    memset(buffer, 0, PAGE_SIZE * 8);
    read(handle,buffer,get_size(handle));
    serial_printf("FILE : %s \n", buffer);
    seek(handle,SEEK_END);
    sched_yield();
    serial_printf("Thread %i back online\n", cpu_no);
    timer_sleep(1500);
    serial_printf("Thread %i exiting\n", cpu_no);
    kfree(buffer);
    sched_exit();
}

