//
// Created by dustyn on 6/21/24.
//

#pragma once
#include <include/definitions/types.h>

#define QUEUE_MODE_FIFO 0 /* Traditional queue */
#define QUEUE_MODE_LIFO 1 // More of a stack but will entertain it
#define QUEUE_MODE_PRIORITY 2 /* Will be used in the scheduler*/
#define QUEUE_MODE_CIRCULAR 3 /* unsure what I'll use this for maybed for packet queues? */
#define QUEUE_MODE_DOUBLE_ENDED 4 /* unsure what I'll use this for but will keep as a placeholder */

/*
 *We want this to be aligned nicely so will need to change it a bit
 */
struct queue_node{
    void* data;
    struct queue_node* next;
    struct queue_node* prev;
    uint8_t priority;
};


struct queue {
    struct queue_node* head;
    struct queue_node* tail;
    //for ID , debugging
    char *name; /* Should be short */
    uint8_t queue_mode; /* Determines the behaviour of enqueue, dequeue, init behaviours to provide maximum flexibility for use elsewhere in the kernel */
    uint32_t node_count;

};

void queue_init(struct queue* queue_head, uint8_t queue_mode, char* name);
void enqueue(struct queue* queue_head, void* data_to_enqueue, uint8_t priority);
void dequeue(struct queue* queue_head);