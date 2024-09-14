//
// Created by dustyn on 9/14/24.
//
/*
	Dustyn's fair scheduler
 */
#include "include/scheduling/dfs.h"
#include "include/data_structures/queue.h"
#include "include/drivers/uart.h"


struct queue dfs_queue;

void dfs_init() {
  queue_init(&dfs_queue,QUEUE_MODE_FIFO,"dfs");
  enqueue(&dfs_queue,(void *)5,0);
  enqueue(&dfs_queue,(void *)&dfs_queue,0);
  enqueue(&dfs_queue,(void *)3,0);
  enqueue(&dfs_queue,(void *)2,0);
  enqueue(&dfs_queue,(void *)1,0);
  serial_printf("dfs head %x.64 dfs tail %x.64\n",dfs_queue.head,dfs_queue.tail);
  dequeue(&dfs_queue);
  struct queue *node = dfs_queue.head->data;
  serial_printf("dfs head %x.8   dfs tail %x.64\n",(struct queue *)node->node_count,dfs_queue.tail);
}
