//
// Created by dustyn on 6/21/24.
//

#include "include/data_structures/queue.h"
#include <include/definitions.h>
#include <include/types.h>

void queue_init(struct queue_node *queue_head, uint8 queue_mode) {
    queue_head->data = NULL;
    queue_head->next = NULL;
    queue_head->prev = NULL;
    queue_head->head = queue_head;
    queue_head->tail = queue_head;
    queue_head->queue_mode = queue_mode;
}


