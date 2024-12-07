//
// Created by dustyn on 12/7/24.
//
#include <stdint.h>
#include "include/scheduling/kthread.h"

#include <string.h>
#include <include/arch/arch_asm_functions.h>
#include <include/filesystem/vfs.h>

#include "include/mem/kalloc.h"
#include "include/scheduling/process.h"

void kthread_init(){
  struct process *proc = _kalloc(sizeof(struct process));
  memset(proc, 0, sizeof(struct process));
  proc->current_cpu = my_cpu();
  proc->current_working_dir = &vfs_root;
  vfs_root.vnode_active_references++;
  proc->handle_list = _kalloc(sizeof(struct virtual_handle_list*));
  proc->handle_list->handle_id_bitmap = 0;
  proc->handle_list->num_handles =0;
  proc->page_map = kernel_pg_map;
  proc->parent_process_id = 0;
  proc->process_type = KERNEL_THREAD;
  proc->current_gpr_state = _kalloc(sizeof(struct gpr_state));
  get_gpr_state(proc->current_gpr_state);

}
