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
#include "include/drivers/uart.h"
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
  for(int i = 0; i < cpu_count; i++) {
    queue_init(&local_run_queues[i],QUEUE_MODE_FIFO,"dfs");
    cpu_list[i].local_run_queue = &local_run_queues[i];
  }
  serial_printf("DFS: Local CPU RQs Initialized \n");

  serial_printf(" 10430503 %i\n",10430503);

}


void dfs_yield() {

}

void dfs_run() {

}


void dfs_preempt() {

}


void dfs_claim_process() {
}