//
// Created by dustyn on 6/21/24.
//

#pragma once
#include <include/types.h>

#define QUEUE_MODE_FIFO 0
#define QUEUE_MODE_LIFO 1 // More of a stack but will entertain it
#define QUEUE_MODE_PRIORITY 2
#define QUEUE_MODE_CIRCULAR 3
#define QUEUE_MODE_DOUBLE_ENDED 4

/*
 *We want this to be aligned nicely so will need to change it a bit
 */
struct queue_node{
    void* data;
    struct queue_node* next;
    struct queue_node* prev;

    /* Head & Tail will only be really tracked from the head and the tail I will not walk the entire queue every operating to update head and tail pointers*/
    struct queue_node* head;
    struct queue_node* tail;

    uint8 queue_mode;
    uint8 priority;
};
