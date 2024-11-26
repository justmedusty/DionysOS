//
// Created by dustyn on 6/21/24.
//

#include "include/data_structures/singly_linked_list.h"

#include <stdlib.h>
#include <include/arch/arch_cpu.h>
#include <include/drivers/serial/uart.h>

#include "include/types.h"
#include <include/mem/kalloc.h>
#include "include/definitions.h"

/*
 *  Generic Implementation of a singly linked list with some functions to operate on them.
 *  I will leave out things like sorts since I am trying to make this as generic as possible and I can't sort without knowing the type the data pointer leads to.
 */

uint8_t static_pool_setup = 0;
uint8_t pool_full = 0;
struct singly_linked_list free_nodes;
struct singly_linked_list_node singly_linked_list_node_static_pool[SINGLY_LINKED_LIST_NODE_STATIC_POOL_SIZE]; /* Since data structures needed during phys_init require list nodes and tree nodes, and they cannot be dynamically allocated yet, we need static pools*/

void singly_linked_list_init(struct singly_linked_list* list,uint64_t flags) {

    if(list == NULL) {
        panic("singly linked list memory allocation failed");
    }

    if(static_pool_setup == 0) {
        static_pool_setup = 1;
        free_nodes.flags = LIST_FLAG_FREE_NODES;
        free_nodes.node_count = 0;
        free_nodes.head = NULL;
        free_nodes.tail = NULL;
        for(uint64_t i = 0; i < SINGLY_LINKED_LIST_NODE_STATIC_POOL_SIZE; i++) {
            singly_linked_list_node_static_pool[i].flags |= STATIC_POOL_NODE;
            singly_linked_list_node_static_pool[i].flags |= STATIC_POOL_FREE_NODE;
            singly_linked_list_insert_head(&free_nodes, &singly_linked_list_node_static_pool[i]);
        }
    }
    list->flags = flags;
    list->head = NULL;
    list->tail = NULL;
    list->node_count = 0;


}

static struct singly_linked_list_node *singly_linked_list_node_alloc() {

    struct singly_linked_list_node* new_node = singly_linked_list_remove_head(&free_nodes);


    if(new_node == NULL) {
        new_node = kalloc(sizeof(struct  singly_linked_list_node));
    }
    new_node->flags &= ~STATIC_POOL_FREE_NODE;
    new_node->data = NULL;

    return new_node;
}

static void singly_linked_list_node_free(struct singly_linked_list_node* node) {

    if(node->flags & STATIC_POOL_NODE) {
        node->flags &= ~STATIC_POOL_FREE_NODE;
        node->data = NULL;
        node->next = NULL;
        singly_linked_list_insert_tail(&free_nodes, node);
        return;
    }

    kfree(node);
}
void singly_linked_list_insert_tail(struct singly_linked_list* list, void* data) {

    struct singly_linked_list_node *new_node;
    if(list->flags & LIST_FLAG_FREE_NODES) {
        new_node = data;
    }else {
        new_node = singly_linked_list_node_alloc();
    }

    if(new_node == NULL) {
        panic("singly_linked_list_insert_tail : Allocation Failure");
    }

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

    struct singly_linked_list_node *new_head;

    if(list->flags & LIST_FLAG_FREE_NODES) {
        new_head = data;
    }else {
        new_head = singly_linked_list_node_alloc();
    }
    if(new_head == NULL) {
        panic("singly_linked_list_insert_tail : Allocation Failure");
    }

    list->node_count++;
    new_head->data = data;
    new_head->next = list->head;
    list->head = new_head;

    struct singly_linked_list_node* pointer = list->head;
    if(list->tail == NULL && (list->node_count > 1)) {
        while(pointer != NULL) {
            if(pointer->next == NULL) {
                list->tail = pointer;
                return;
            }
            pointer = pointer->next;
        }

    }

}


void *singly_linked_list_remove_tail(struct singly_linked_list* list) {

    if(list->node_count == 0) {
        return NULL;
    }

    void *return_value;

    if(list->tail != NULL) {
        return_value = list->tail->data;
    }

    if(list->node_count == 1) {
        return_value = list->head->data;
        singly_linked_list_node_free(list->head);
        list->head = NULL;
        list->tail = NULL;
        list->node_count--;
        return return_value;
    }

    struct singly_linked_list_node* node = list->head;
    singly_linked_list_node_free(list->tail);
    list->tail = NULL;

    while (node != NULL) {
        if(node->next == NULL) {
            list->tail = node;
        }
        node = node->next;
    }

    list->node_count--;
    return return_value;

}
uint64_t counter = 0;
void *singly_linked_list_remove_head(struct singly_linked_list* list) {
    counter++;
    if (list->node_count == 0) {
        return NULL;
    }
    void *return_value = list->head->data;

    if(list->flags & LIST_FLAG_FREE_NODES) {
        list->head = list->head->next;
        list->node_count--;
        return return_value;
    }

    if(list->node_count == 1) {
        singly_linked_list_node_free(list->head);
        list->head = NULL;
        list->tail = NULL;
        list->node_count--;
        return return_value;
    }
    if(list->head->next == NULL && list->node_count > 1) {
        serial_printf("Fucked up shit happens in call %i\n",counter);
        list->node_count = 1;
    }
    struct singly_linked_list_node* new_head = list->head->next;

    singly_linked_list_node_free(list->head);
    list->head = new_head;
    list->node_count--;
    return return_value;
}

uint64_t singly_linked_list_remove_node_by_address(struct singly_linked_list* list, void* data) {
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
            singly_linked_list_node_free(node);
            return SUCCESS;
        }

        node = node->next;
        prev = prev->next;
    }

    return NODE_NOT_FOUND;
}