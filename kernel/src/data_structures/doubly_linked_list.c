//
// Created by dustyn on 9/11/24.
//

#include "doubly_linked_list.h"
#include <include/kalloc.h>
#include "include/definitions.h"

void doubly_linked_list_init(struct doubly_linked_list_node *list) {
    list->next = NULL;
    list->prev = NULL;
    list->data = NULL;
}

void doubly_linked_list_insert_tail(struct doubly_linked_list_node* head, void* data) {
    struct doubly_linked_list_node* node = head;

    while (node->next != NULL) {
        node = node->next;
    }

    node->next = kalloc(sizeof(struct doubly_linked_list_node));
    node->next->data = data;
}

struct doubly_linked_list_node* doubly_linked_list_insert_head(struct doubly_linked_list_node* old_head, void* data) {
    struct doubly_linked_list_node* new_head = kalloc(sizeof(struct doubly_linked_list_node));
    new_head->data = data;
    new_head->next = old_head;
    return new_head;
}


void doubly_linked_list_remove_tail(struct doubly_linked_list_node* head) {
    struct doubly_linked_list_node* node = head;
    struct doubly_linked_list_node* new_tail = NULL;

    if(node->next == NULL) {
        kfree(node);
        return;
    }

    while (node->next != NULL) {
        node = node->next;
        if (node->next->next == NULL) {
            new_tail = node->next;
            node = node->next;
            break;
        }
    }
    new_tail->next = NULL;
    kfree(node);
}

struct doubly_linked_list_node* singled_linked_list_remove_head(struct doubly_linked_list_node* head) {
    if (head->next == NULL) {
        kfree(head);
        return NULL;
    }

    struct doubly_linked_list_node* new_head = head->next;
    kfree(head);
    return new_head;
}