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
    struct process *proc = kzmalloc(sizeof(struct process));

    proc->current_cpu = my_cpu();

    proc->current_working_dir = &vfs_root;

    vfs_root.vnode_active_references++;

    proc->handle_list = kmalloc(sizeof(struct virtual_handle_list));

    proc->handle_list->handle_list = kmalloc(sizeof(struct doubly_linked_list));

    doubly_linked_list_init(proc->handle_list->handle_list);

    proc->handle_list->handle_id_bitmap = 0;
    proc->handle_list->num_handles = 0;

    struct virt_map *kthread_map = kmalloc(sizeof(struct virt_map));
    kthread_map->top_level = kernel_pg_map->top_level;
    proc->page_map = kthread_map;

    proc->parent_process_id = 0;
    proc->process_type = KERNEL_THREAD;
    proc->current_register_state = kmalloc(sizeof(struct register_state));
    memset(proc->current_register_state, 0, sizeof(struct register_state));

    /*
     * Create dedicated region, share kernel page table but have their own region for stack
     */
    void *stack = kmalloc(DEFAULT_STACK_SIZE);
    /*
     * A lot of the flags here are not actually needed since there is no new mappings
     */
    struct virtual_region *stack_region = create_region((uint64_t) stack, DEFAULT_STACK_SIZE / PAGE_SIZE, STACK, READWRITE, true);
    attach_region(proc->page_map,stack_region);
    proc->stack =stack;

    /*
     * Set the architecture specific registers ahead of time the stack is set up as well as the instruction pointer
     * The instruction pointer points to kthread main
     */
#ifdef __x86_64__
    proc->current_register_state->rip = (uint64_t) kthread_main; // it's grabbing a junk value if not called from an interrupt so overwriting rip with kthread main
    proc->current_register_state->rsp = (uintptr_t)(proc->stack )+ DEFAULT_STACK_SIZE; /* Allocate a private stack */
    proc->current_register_state->rbp = proc->current_register_state->rsp - 8; /* Set base pointer to the new stack pointer, -8 for return address */
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
    char *buffer = kmalloc(PAGE_SIZE);
    int64_t handle = open("/etc/passwd");
    DEBUG_PRINT("HANDLE %i\n",handle);
    if(handle < 0){
        goto done;
    }
    memset(buffer, 0, PAGE_SIZE * 8);
    int64_t ret = read(handle,buffer,get_size(handle));

    if(ret != KERN_SUCCESS){
        serial_printf("Error opening file passwd!\n");
        goto done;
    }
    int64_t handle2 = open("/home/welcome.txt");
    DEBUG_PRINT("HANDLE %i\n",handle);
    if(handle2 < 0){
        goto done;
    }
    memset(buffer, 0, PAGE_SIZE * 8);
    ret = read(handle2,buffer,get_size(handle2));

    if(ret != KERN_SUCCESS){
        serial_printf("Error opening file passwd!\n");
        goto done;
    }
    DEBUG_PRINT("FILE : %s RET: %i \n", buffer,ret);
    int64_t handle3 = open("/home/cool_quotes.txt");
    DEBUG_PRINT("HANDLE %i\n",handle);
    if(handle3 < 0){
        goto done;
    }
    memset(buffer, 0, PAGE_SIZE * 8);
    ret = read(handle3,buffer,get_size(handle2));

    if(ret != KERN_SUCCESS){
        serial_printf("Error opening file passwd!\n");
        goto done;
    }
    DEBUG_PRINT("FILE : %s RET: %i \n", buffer,ret);
    seek(handle,SEEK_END);
    seek(handle2,SEEK_END);
    seek(handle3,SEEK_END);
    done:
    sched_yield();
    serial_printf("Thread %i back online\n", cpu_no);
    timer_sleep(10);
    serial_printf("Thread %i exiting\n", cpu_no);
    kfree(buffer);
    sched_exit();
}

