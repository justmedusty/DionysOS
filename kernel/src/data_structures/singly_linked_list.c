//
// Created by dustyn on 6/21/24.
//

#include "include/data_structures/singly_linked_list.h"

#include <math.h>
#include <stdlib.h>
#include "include/definitions/string.h"
#include <include/architecture/arch_cpu.h>
#include <include/drivers/serial/uart.h>

#include "include/definitions/types.h"
#include <include/memory/kalloc.h>
#include "include/definitions/definitions.h"

/*
 *  Generic Implementation of a singly linked list with some functions to operate on them.
 *  I will leave out things like sorts since I am trying to make this as generic as possible and I can't sort without knowing the type the data pointer leads to.
 */

struct spinlock sll_lock;
uint8_t static_pool_setup = 0;
uint8_t pool_full = 0;
struct singly_linked_list free_nodes;
struct singly_linked_list_node singly_linked_list_node_static_pool[SINGLY_LINKED_LIST_NODE_STATIC_POOL_SIZE]; /* Since data structures needed during phys_init require list nodes and tree nodes, and they cannot be dynamically allocated yet, we need static pools*/

void singly_linked_list_init(struct singly_linked_list* list,uint64_t flags) {

    if(list == NULL) {
        panic("singly linked list memory allocation failed");
    }
    initlock(&list->lock,SINGLE_LINKED_LIST_LOCK);
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
    acquire_spinlock(&sll_lock);
    struct singly_linked_list_node* new_node = singly_linked_list_remove_head(&free_nodes);


    if(new_node == NULL) {
        new_node =_kalloc(sizeof(struct  singly_linked_list_node));
    }
    new_node->flags &= ~STATIC_POOL_FREE_NODE;
    new_node->data = NULL;
    release_spinlock(&sll_lock);
    return new_node;
}

static void singly_linked_list_node_free(struct singly_linked_list_node* node) {
    acquire_spinlock(&sll_lock);
    if(node->flags & STATIC_POOL_NODE) {
        node->flags &= ~STATIC_POOL_FREE_NODE;
        node->data = NULL;
        node->next = NULL;
        singly_linked_list_insert_tail(&free_nodes, node);
        release_spinlock(&sll_lock);
        return;
    }
    memset(node, 0, sizeof(struct singly_linked_list_node));
    _kfree(node);
    release_spinlock(&sll_lock);
}
//NOTE Do not call this if there is allocated memory to which you only have the one reference! It will be leaked!
void singly_linked_list_destroy(struct singly_linked_list *list){
    struct singly_linked_list_node *pointer = list->head;
    for (size_t i = 0; i < list->node_count; ++i) {
        if(pointer == NULL){
            return;
        }
        struct singly_linked_list_node *current = pointer;
        pointer = pointer->next;
        singly_linked_list_node_free(current);
    }
}

void singly_linked_list_insert_tail(struct singly_linked_list* list, void* data) {
    acquire_spinlock(&list->lock);
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
    new_node->next = NULL;

    if(list->tail == NULL && list->node_count == 1) {
        list->head->next = new_node;
        list->tail = new_node;
        list->node_count++;
        release_spinlock(&list->lock);
        return;
    }

    list->tail->next = new_node;
    list->tail = new_node;
    list->node_count++;
    release_spinlock(&list->lock);

}

void singly_linked_list_insert_head(struct singly_linked_list* list, void* data) {
    acquire_spinlock(&list->lock);
    struct singly_linked_list_node *new_head;

    if(list->flags & LIST_FLAG_FREE_NODES) {
        new_head = data;
    }else {
        new_head = singly_linked_list_node_alloc();
    }

    if(new_head == NULL) {
        panic("singly_linked_list_insert_tail : Allocation Failure");
    }
    new_head->data = data;

    if (list->node_count == 0) {
        list->head = new_head;
        new_head->next = NULL;
        list->tail = NULL;
        list->node_count++;
        release_spinlock(&list->lock);
        return;
    }

    if (list->node_count == 1) {
        list->node_count++;
        new_head->data = data;
        new_head->next = list->head;
        list->head = new_head;
        list->tail = new_head->next;
        list->tail->next = NULL;
        release_spinlock(&list->lock);
        return;
    }

    list->node_count++;
    new_head->data = data;
    new_head->next = list->head;
    list->head = new_head;
    release_spinlock(&list->lock);

}


void *singly_linked_list_remove_tail(struct singly_linked_list* list) {
    acquire_spinlock(&list->lock);
    if(list->node_count == 0) {
        release_spinlock(&list->lock);
        return NULL;
    }

    void *return_value;

    if(list->tail != NULL) {
        return_value = list->tail->data;
    }

    if(list->node_count == 1) {
        return_value = list->head->data;
        list->head->next = NULL;
        singly_linked_list_node_free(list->head);
        list->head = NULL;
        list->tail = NULL;
        list->node_count--;
        release_spinlock(&list->lock);
        return return_value;
    }

    struct singly_linked_list_node* node = list->head;
    struct singly_linked_list_node* tail = list->tail;
    while (node != NULL) {
        if(node->next == list->tail) {
            list->tail = node;
            node->next = NULL;
        }
        node = node->next;
    }

    list->node_count--;
    tail->next = NULL;
    singly_linked_list_node_free(tail);
    release_spinlock(&list->lock);
    return return_value;

}

void *singly_linked_list_remove_head(struct singly_linked_list* list) {
    acquire_spinlock(&list->lock);
    if (list->node_count == 0 || list->head == NULL) {
        release_spinlock(&list->lock);
        return NULL;
    }

    void *return_value = list->head->data;

    if(list->flags & LIST_FLAG_FREE_NODES) {
        list->head = list->head->next;
        list->node_count--;
        release_spinlock(&list->lock);
        return return_value;
    }

    if(list->node_count == 1) {
        singly_linked_list_node_free(list->head);
        list->head = NULL;
        list->tail = NULL;
        list->node_count--;
        release_spinlock(&list->lock);
        return return_value;
    }
    if (list->node_count == 2) {
        struct singly_linked_list_node* node = list->head;
        list->head = list->tail;
        list->head->next = NULL;
        list->tail = NULL;
        list->node_count--;
        node->next = NULL;
        singly_linked_list_node_free(node);
        release_spinlock(&list->lock);
        return return_value;
    }

    struct singly_linked_list_node* new_head = list->head->next;

    singly_linked_list_node_free(list->head);
    list->head = new_head;
    list->node_count--;
    release_spinlock(&list->lock);
    return return_value;
}

uint64_t singly_linked_list_remove_node_by_address(struct singly_linked_list* list, void* data) {
    acquire_spinlock(&list->lock);
    struct singly_linked_list_node* prev = list->head;
    struct singly_linked_list_node* node = list->head;

    if (data == NULL) {
        serial_printf("singly_linked_list_remove_node_by_address : data is NULL\n");
    }
    if(node->data == data) {
        release_spinlock(&list->lock);
        singly_linked_list_remove_head(list);
        return SUCCESS;
    }

    node = node->next; // Handle head and increment node pointer so that we can track prev properly

    while (node != NULL) {
        if(node->data == data) {
            prev->next = node->next;
            singly_linked_list_node_free(node);
            list->node_count--;
            release_spinlock(&list->lock);
            return SUCCESS;
        }

        node = node->next;
        prev = prev->next;
    }
    release_spinlock(&list->lock);
    return NODE_NOT_FOUND;
}