//
// Created by dustyn on 9/11/24.
//

#pragma once
#include "include/types.h"

struct doubly_linked_list_node{
    void* data;
    struct doubly_linked_list_node* next;
    struct doubly_linked_list_node* prev;
};

struct doubly_linked_list {
    struct doubly_linked_list_node* head;
    struct doubly_linked_list_node* tail;
    uint64 node_count;
};

void doubly_linked_list_init(struct doubly_linked_list *list);
void doubly_linked_list_insert_tail(struct doubly_linked_list *list, void* data);
void doubly_linked_list_insert_head(struct doubly_linked_list *list, void* data);
void doubly_linked_list_remove_tail(struct doubly_linked_list* list);
void doubly_linked_list_remove_head(struct doubly_linked_list* list);


