//
// Created by dustyn on 12/7/24.
//
#include <stdint.h>
#include "include/scheduling/kthread.h"

#include <string.h>
#include <include/arch/arch_asm_functions.h>
#include <include/data_structures/doubly_linked_list.h>
#include <include/filesystem/vfs.h>
#include <include/scheduling/sched.h>

#include "include/mem/kalloc.h"
#include "include/scheduling/process.h"
/*
 * Initialize a kthread and add it to the local run-queue
 */
void kthread_init() {

  struct process* proc = kmalloc(sizeof(struct process));
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
  proc->current_gpr_state = kmalloc(sizeof(struct gpr_state));
  memset(proc->current_gpr_state, 0, sizeof(struct gpr_state));
  proc->current_gpr_state->rip = (uint64_t)kthread_main; // it's grabbing a junk value if not called from an interrupt so overwriting rip with kthread main
  proc->current_gpr_state->rsp = (uint64_t)kmalloc(DEFAULT_STACK_SIZE) + DEFAULT_STACK_SIZE; /* Allocate a private stack */
  proc->current_gpr_state->rbp = proc->current_gpr_state->rsp - 8; /* Set base pointer to the new stack pointer, -8 for return address*/
  proc->current_gpr_state->interrupts_enabled = are_interrupts_enabled();

  if (proc->current_cpu->local_run_queue == NULL) {
    proc->current_cpu->local_run_queue = &local_run_queues[proc->current_cpu->cpu_number];
  }

  enqueue(proc->current_cpu->local_run_queue, proc, MEDIUM);
}
/*
 * For the time being, this function can't return
 */
void kthread_main() {
  serial_printf("kthread active on cpu %i\n", my_cpu()->cpu_number);
  serial_printf("do stuff\n");
  sched_exit();
}

