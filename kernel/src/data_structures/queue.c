//
// Created by dustyn on 6/21/24.
//

#include "include/data_structures/queue.h"

#include <stdlib.h>
#include <include/definitions.h>
#include <include/kalloc.h>
#include <include/types.h>
#include <include/arch/arch_cpu.h>

/*
 *  Going to make this a very broad generic data structure in order to make it useful for many different scenarios. I will probably need to be careful with
 *  things like updating head and tail with a packed queue. I can probably only work witht he head and tail and only traverse when needed for something like a
 *  priority queue.
 */

void queue_init(struct queue_node* queue_head, uint8 queue_mode, void* data,
                uint8 priority /* Only relevant for a priority queue */) {
    queue_head->data = data;
    queue_head->next = NULL;
    queue_head->prev = NULL;
    queue_head->head = queue_head;
    queue_head->tail = queue_head;
    queue_head->queue_mode = queue_mode;
    queue_head->priority = priority;
}

/*
 *  Generic enqueue
 */

void enqueue(struct queue_node* queue_head, void* data_to_enqueue, uint8 priority) {
    uint8 queue_head_empty = 0;
    if (queue_head == NULL) {
        //So i can investigate when it happens, should have null data field not the entire head structure null
        panic("Null head");
    }

    struct queue_node* new_node = kalloc(sizeof(struct queue_node));
    queue_init(new_node, queue_head->queue_mode, data_to_enqueue, priority);


    switch (queue_head->queue_mode) {
    case QUEUE_MODE_FIFO:
        break;

    case QUEUE_MODE_LIFO:
        break;

    case QUEUE_MODE_PRIORITY:
        break;

    case QUEUE_MODE_CIRCULAR:
        break;

    case QUEUE_MODE_DOUBLE_ENDED:
        break;

    default:
        break;
    }
}

/*
 *  Generic dequeue
 */
void dequeue(struct queue_node* queue_head) {
    if (queue_head == NULL) {
        //So i can investigate when it happens, should have null data field not the entire head structure null
        panic("Null head");
    }

    switch (queue_head->queue_mode) {
    case QUEUE_MODE_FIFO:
        break;

    case QUEUE_MODE_LIFO:
        break;

    case QUEUE_MODE_PRIORITY:
        break;

    case QUEUE_MODE_CIRCULAR:
        break;

    case QUEUE_MODE_DOUBLE_ENDED:
        break;

    default:
        break;
    }
}


void __enqueue_fifo(struct queue_node* queue_head, struct queue_node* new_node) {
    /*
     * Base cases
     */

    //We will keep an empty queue head when the queue is empty , this makes more sense than throwing it out every time and reallocing.
    if (queue_head->data == NULL) {
        kfree(queue_head);
        queue_head = new_node;
        return;
    }

    if (queue_head->next == NULL) {
        queue_head->next = new_node;
        new_node->prev = queue_head;
        queue_head->tail = new_node;

        return;
    }


    queue_head->tail->next = new_node;
    new_node->prev = queue_head->tail;
    queue_head->tail = new_node;
    new_node->head = queue_head->tail;
}

/* Push */

/*
 *  Should I even bother with tail maintenance here? Unsure it could help if I want to mark pseudo frames or something. I'll sit on it for a bit.
 */
void __enqueue_lifo(struct queue_node* queue_head, struct queue_node* new_node) {
    /*
    * Base cases
    */

    if (queue_head->data == NULL) {
        kfree(queue_head);
        queue_head = new_node;
        return;
    }

    new_node->next = queue_head;
    queue_head->prev = new_node;
    new_node->tail = queue_head->tail;
    queue_head = new_node;
}

void __enqueue_priority(struct queue_node* queue_head, struct queue_node* new_node) {

    if (queue_head->data == NULL) {
        kfree(queue_head);
        queue_head = new_node;
        return;
    }
    struct queue_node* pointer = queue_head;

    for(;;) {

        if (pointer->next == NULL) {
            break;
        }

        if (pointer->priority < new_node->priority) {

        }


    }
}
