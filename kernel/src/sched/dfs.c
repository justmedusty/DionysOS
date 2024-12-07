//
// Created by dustyn on 9/14/24.
//

/*
	Dustyn's fair scheduler
 */


#include "include/scheduling/dfs.h"

#include <include/definitions.h>
#include <include/data_structures/spinlock.h>

#include "include/data_structures/queue.h"
#include "include/drivers/serial/uart.h"
#include "include/arch/arch_smp.h"
#include "include/arch/arch_cpu.h"
#include "include/definitions/string.h"
#include <include/mem/kalloc.h>
#include <include/mem/mem.h>


struct queue dfs_global_queue;
struct spinlock dfs_global_lock;

void dfs_init() {
  initlock(&dfs_global_lock, DFS_LOCK);
  queue_init(&dfs_global_queue,QUEUE_MODE_FIFO,"dfs_global");
  for(uint32_t i = 0; i < cpu_count; i++) {
    queue_init(&local_run_queues[i],QUEUE_MODE_FIFO,"dfs");
    cpu_list[i].local_run_queue = &local_run_queues[i];
  }
  serial_printf("DFS: Local CPU RQs Initialized \n");
}


void dfs_yield() {
  struct process *process = my_cpu()->running_process;
  get_regs(process->current_gpr_state);
  enqueue(my_cpu()->local_run_queue,process,process->priority);
  dfs_run();
}

void dfs_run() {
  struct cpu *cpu = my_cpu();
  if (cpu->running_process == NULL) {
    //handle empty queue
  }
  cpu->running_process = cpu->local_run_queue->head->data;

  dequeue(cpu->local_run_queue);

  restore_execution(cpu->running_process->current_gpr_state);
}


void dfs_preempt() {
  struct process *process = my_cpu()->running_process;
  get_regs(process->current_gpr_state);
  enqueue(my_cpu()->local_run_queue,process,process->priority);
  dfs_run();
}

void dfs_claim_process() {
}
