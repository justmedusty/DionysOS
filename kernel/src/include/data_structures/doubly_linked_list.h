//
// Created by dustyn on 9/11/24.
//

#pragma once
#include "spinlock.h"


struct doubly_linked_list_node{
    void* data;
    struct doubly_linked_list_node* next;
    struct doubly_linked_list_node* prev;
};

struct doubly_linked_list {
    struct spinlock lock;
    struct doubly_linked_list_node* head;
    struct doubly_linked_list_node* tail;
    uint64_t node_count;
};

void doubly_linked_list_init(struct doubly_linked_list *list);
void doubly_linked_list_insert_tail(struct doubly_linked_list *list, void* data);
void doubly_linked_list_insert_head(struct doubly_linked_list *list, void* data);
void doubly_linked_list_remove_tail(struct doubly_linked_list* list);
void doubly_linked_list_remove_head(struct doubly_linked_list* list);
void doubly_linked_list_remove_node_by_address(struct doubly_linked_list *list,struct doubly_linked_list_node* node);
void doubly_linked_list_destroy(struct doubly_linked_list* list);

