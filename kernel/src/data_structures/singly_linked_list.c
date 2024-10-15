//
// Created by dustyn on 6/21/24.
//

#include "include/data_structures/singly_linked_list.h"

#include <include/arch/arch_cpu.h>

#include "include/types.h"
#include <include/mem/kalloc.h>

#include "include/definitions.h"

/*
 *  Generic Implementation of a singly linked list with some functions to operate on them.
 *  I will leave out things like sorts since I am trying to make this as generic as possible and I can't sort without knowing the type the data pointer leads to.
 */

void singly_linked_list_init(struct singly_linked_list* list) {
    list->head = NULL;
    list->tail = NULL;
    list->node_count = 0;
}

void singly_linked_list_insert_tail(struct singly_linked_list* list, void* data) {

    struct singly_linked_list_node *new_node = kalloc(sizeof(struct singly_linked_list_node));
    new_node->data = data;

    if(list->tail == NULL) {
        list->head = new_node;
        list->tail = new_node;
        list->node_count++;
        return;
    }

    list->tail->next = new_node;
    list->tail = new_node;
    list->node_count++;


}

void singly_linked_list_insert_head(struct singly_linked_list* list, void* data) {
    struct singly_linked_list_node* new_head = kalloc(sizeof(struct singly_linked_list_node));
    new_head->data = data;
    new_head->next = list->head;
    list->head = new_head;
    list->node_count++;
}


void singly_linked_list_remove_tail(struct singly_linked_list* list) {

    if(list->node_count == 0) {
        return;
    }

    if(list->node_count == 1) {
        kfree(list->head);
        list->head = NULL;
        list->tail = NULL;
        list->node_count--;
        return;
    }

    struct singly_linked_list_node* node = list->head;
    kfree(list->tail);
    list->tail = NULL;

    while (node != NULL) {
        if(node->next == NULL) {
            list->tail = node;

        }
        node = node->next;
    }

    list->node_count--;
}

void singly_linked_list_remove_head(struct singly_linked_list* list) {
    if (list->node_count == 0) {
        return;
    }

    if(list->node_count == 1) {
        kfree(list->head);
        list->head = NULL;
        list->tail = NULL;
        list->node_count--;
        return;
    }

    struct singly_linked_list_node* new_head = list->head->next;
    kfree(list->head);
    list->head = new_head;
    list->node_count--;
}

uint64 singly_linked_list_remove_node_by_address(struct singly_linked_list* list, void* data) {
    struct singly_linked_list_node* prev = list->head;
    struct singly_linked_list_node* node = list->head;

    if(node->data == data) {
        singly_linked_list_remove_head(list);
        return SUCCESS;
    }

    node = node->next; // Handle head and increment node pointer so that we can track prev properly

    while (node != NULL) {
        if(node->data == data) {
            prev->next = node->next;
            kfree(node);
            return SUCCESS;
        }

        node = node->next;
        prev = prev->next;
    }
    return NODE_NOT_FOUND;
}