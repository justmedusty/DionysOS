//
// Created by dustyn on 9/14/24.
//

/*
	Dustyn's fair scheduler
 */
#define _DFS_ // this is only here because clion is complaining and I lose intellisense because reading a makefile is too difficult and complicated. This is meant to just be defined in the makefile

#include <include/data_structures/doubly_linked_list.h>
#ifdef _DFS_


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


struct queue sched_global_queue;
struct spinlock sched_global_lock;

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
  initlock(&sched_global_lock, sched_LOCK);
  queue_init(&sched_global_queue,QUEUE_MODE_FIFO,"sched_global");
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
  get_regs(process->current_gpr_state);
  enqueue(my_cpu()->local_run_queue,process,process->priority);
  context_switch(my_cpu()->running_process->current_gpr_state,my_cpu()->scheduler_state);
}

void sched_run() {
  struct cpu *cpu = my_cpu();

  if (cpu->local_run_queue->head == NULL) {
    serial_printf("Do busy work, poach processes etc\n");
    return;
  }

  cpu->running_process = cpu->local_run_queue->head->data;
  cpu->running_process->current_state = PROCESS_RUNNING;
  dequeue(cpu->local_run_queue);

  context_switch(my_cpu()->scheduler_state,cpu->running_process->current_gpr_state);
  cpu->running_process->current_state = PROCESS_READY;
}


void sched_preempt() {
  struct process *process = my_cpu()->running_process;
  get_regs(process->current_gpr_state);
  enqueue(my_cpu()->local_run_queue,process,process->priority);
  context_switch(my_cpu()->running_process->current_gpr_state,my_cpu()->scheduler_state);
}

void sched_claim_process() {
}

void sched_exit() {
  struct process *process = my_cpu()->running_process;
  my_cpu()->running_process = NULL;
  free_process(process);
  context_switch(my_cpu()->running_process->current_gpr_state,my_cpu()->scheduler_state);
}
#endif