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
 *  The idea behind all of this here is to create a generic and flexible queue interface with many different modes.
 *
 *  FIFO (classical queue) mode
 *  LIFO (stack) mode
 *  Priority (scheduling) mode
 *  Circular (maybe packet buffering , not sure yet)
 *  Double ended (not sure yet)
 *
 *  Note for clarity : the direction of next and prev amongst nodes in a queue
 *
 *  HEAD->next->next->next->TAIL
 *  TAIL->prev->prev->prev->HEAD
 */



/*
 * Init a queue
 */
void queue_init(struct queue* queue_head, uint8 queue_mode, char *name) {
    queue_head->name = name;
    queue_head->head = NULL;
    queue_head->tail = NULL;
    queue_head->queue_mode = queue_mode;
}

/*
 *  Init a single node
 */
void queue_node_init(struct queue_node *node,void *data, uint8 priority) {
    if (node == NULL) {
        //I'll use panics to easily figure out if these things happen and can work from there if it ever is an issue
        panic("queue_node_init called with NULL node");
    }

    node->data = data;
    node->priority = priority;
    node->next = NULL;
    node->prev = NULL;
}

/*
 *  Generic enqueue
 */

void enqueue(struct queue* queue_head, void* data_to_enqueue, uint8 priority) {
    uint8 queue_head_empty = 0;
    if (queue_head == NULL) {
        //So i can investigate when it happens, should have null data field not the entire head structure null
        panic("Null head");
    }

    struct queue_node* new_node = kalloc(sizeof(struct queue_node));
    queue_node_init(new_node,data_to_enqueue,priority);


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
void dequeue(struct queue* queue_head) {
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


void __enqueue_fifo(struct queue* queue_head, struct queue_node* new_node) {
    /*
     * Base cases
     */
    if(new_node == NULL) {
        panic("Null head enqueue fifo");
    }

    if (new_node->data == NULL) {
        panic("Null data enqueue fifo");
        kfree(new_node);
        return;
    }

    if(queue_head->head == NULL) {
        queue_head->head = new_node;
        return;
    }

    if(queue_head->tail == queue_head->head) {
        queue_head->head->next = new_node;
        new_node->prev = queue_head->head;
        queue_head->tail = new_node;
        return;
    }

    queue_head->tail->next = new_node;
    new_node->prev = queue_head->tail;
    queue_head->tail = new_node;
}

/* Push */

void __enqueue_lifo(struct queue* queue_head, struct queue_node* new_node) {
    /*
    * Base cases
    */

    if (new_node->data == NULL) {
        panic("Null head in enqueue lifo");
        kfree(queue_head);
        return;
    }

    if(queue_head->head == NULL) {
        queue_head->head = new_node;
        return;
    }


    queue_head->head->prev = new_node;
    queue_head->head = new_node;
}

void __enqueue_priority(struct queue* queue_head, struct queue_node* new_node) {

    if (new_node->data == NULL) {
        panic("Null head in enqueue priority");
        kfree(queue_head);
        return;
    }
    struct queue_node* pointer = queue_head->head;

    for(;;) {

        if (pointer->next == NULL) {
            break;
        }

        if (pointer->priority < new_node->priority) {

        }


    }
}
