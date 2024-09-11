//
// Created by dustyn on 9/11/24.
//

#pragma once

struct doubly_linked_list_node{
    void* data;
    struct doubly_linked_list_node* next;
    struct doubly_linked_list_node* prev;
};

void doubly_linked_list_init(struct doubly_linked_list_node *list);
void doubly_linked_list_insert_tail(struct doubly_linked_list_node* head, void* data);
struct doubly_linked_list_node* doubly_linked_list_insert_head(struct doubly_linked_list_node* old_head, void* data);
void doubly_linked_list_remove_tail(struct doubly_linked_list_node* head);
struct doubly_linked_list_node* doubly_linked_list_remove_head(struct doubly_linked_list_node* head);


