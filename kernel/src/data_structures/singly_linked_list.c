//
// Created by dustyn on 6/21/24.
//

#include "include/data_structures/singly_linked_list.h"

#include <include/arch/arch_cpu.h>
#include <include/drivers/uart.h>

#include "include/types.h"
#include <include/mem/kalloc.h>
#include "include/definitions.h"

/*
 *  Generic Implementation of a singly linked list with some functions to operate on them.
 *  I will leave out things like sorts since I am trying to make this as generic as possible and I can't sort without knowing the type the data pointer leads to.
 */

uint8 static_pool_setup = 0;

struct singly_linked_list_node singly_linked_list_node_static_pool[SINGLY_LINKED_LIST_NODE_STATIC_POOL_SIZE]; /* Since data structures needed during phys_init require list nodes and tree nodes, and they cannot be dynamically allocated yet, we need static pools*/

void singly_linked_list_init(struct singly_linked_list* list) {

    if(list == NULL) {
        panic("singly linked list memory allocation failed");
    }


    if(static_pool_setup == 0) {
        for(uint64 i = 0; i < SINGLY_LINKED_LIST_NODE_STATIC_POOL_SIZE; i++) {
            singly_linked_list_node_static_pool[i].flags |= STATIC_POOL_NODE;
            singly_linked_list_node_static_pool[i].flags |= STATIC_POOL_FREE_NODE;
        }

        static_pool_setup = 1;
    }

    list->head = NULL;
    list->tail = NULL;
    list->node_count = 0;


}

static struct singly_linked_list_node *singly_linked_list_node_alloc() {

    uint32 index = 0;
    struct singly_linked_list_node* new_node = &singly_linked_list_node_static_pool[index];

    /* Might make this quicker later but this is fine for now */
    while(!(new_node->flags & STATIC_POOL_FREE_NODE)) {

        if(index > SINGLY_LINKED_LIST_NODE_STATIC_POOL_SIZE) {
            new_node = NULL;
            break;
        }

        index++;
        new_node = &singly_linked_list_node_static_pool[index];
    }
    if(new_node == NULL) {
        new_node = kalloc(sizeof(struct  singly_linked_list_node));
    }
    new_node->flags &= ~STATIC_POOL_FREE_NODE;
    new_node->data = NULL;
    return new_node;
}

static void singly_linked_list_node_free(struct singly_linked_list_node* node) {

    if(node->data == (void *) 0x1) {
        panic("");
    }
    if(node->flags & STATIC_POOL_NODE) {
        node->flags &= ~STATIC_POOL_FREE_NODE;
        node->data = NULL;
        node->next = NULL;
        return;
    }

    kfree(node);
}
void singly_linked_list_insert_tail(struct singly_linked_list* list, void* data) {

    struct singly_linked_list_node *new_node = singly_linked_list_node_alloc();

    if(new_node == NULL) {
        panic("singly_linked_list_insert_tail : Allocation Failure");
    }
    if(data == (void *) 0x1) {
        panic("singly_linked_list_insert_tail : Data passed is NULL");
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
    struct singly_linked_list_node* new_head = singly_linked_list_node_alloc();
    if(new_head == NULL) {
        panic("singly_linked_list_insert_tail : Allocation Failure");
    }
    if(data == (void *)0x1) {
        serial_printf("here");
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
    serial_printf("%i node count \n",list->node_count);
    return return_value;

}

void *singly_linked_list_remove_head(struct singly_linked_list* list) {
    if (list->node_count == 0) {
        return NULL;
    }
    void *return_value = list->head->data;

    if(list->node_count == 1) {
        singly_linked_list_node_free(list->head);
        list->head = NULL;
        list->tail = NULL;
        list->node_count--;
        return return_value;
    }

    struct singly_linked_list_node* new_head = list->head->next;
    singly_linked_list_node_free(list->head);
    list->head = new_head;
    list->node_count--;
    return return_value;
    serial_printf("%i node count \n",list->node_count);
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
            singly_linked_list_node_free(node);
            return SUCCESS;
        }

        node = node->next;
        prev = prev->next;
    }
    return NODE_NOT_FOUND;
}