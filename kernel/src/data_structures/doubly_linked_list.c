//
// Created by dustyn on 9/11/24.
//

#include "include/data_structures/doubly_linked_list.h"
#include "include/types.h"
#include <include/mem/kalloc.h>
#include "include/definitions.h"

/*
 * A simple doubly linked list implementation which can be used anywhere in the kernel later on.
* I added a simple node_count since I think this will be useful to have at some point or other.
 */

void doubly_linked_list_init(struct doubly_linked_list *list) {
    list->head = NULL;
    list->tail = NULL;
    list->node_count = 0;
}

void doubly_linked_list_insert_tail(struct doubly_linked_list* list, void* data) {

    struct doubly_linked_list_node* new_node = kalloc(sizeof(struct doubly_linked_list_node));;
    new_node->data = data;

    if(list->node_count == 0) {
      list->head = new_node;
      list->tail = new_node;
      list->node_count++;
      return;
    }

     if(list->node_count == 1) {
       list->tail = new_node;
       list->head->next = new_node;
       list->tail->prev = list->head;
       list->node_count++;
       return;
     }

     list->tail->next = new_node;
     new_node->prev = list->tail;
     list->tail = new_node;
     list->node_count++;
     return;

}

void doubly_linked_list_insert_head(struct doubly_linked_list* list, void* data) {
    struct doubly_linked_list_node* new_head = kalloc(sizeof(struct doubly_linked_list_node));
    new_head->data = data;

    if(list->node_count == 0) {
      list->head = new_head;
      list->tail = new_head;
      list->node_count++;
      return;
    }

    new_head->next = list->head;
    list->head->prev = new_head;
    list->head = new_head;
    list->node_count++;
    return;
}

void doubly_linked_list_remove_tail(struct doubly_linked_list* list) {

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

     list->tail = list->tail->prev;
     kfree(list->tail->next);
     list->tail->next = NULL;
     list->node_count--;
     return;
}

void doubly_linked_list_remove_head(struct doubly_linked_list* list) {

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

    list->head = list->head->next;
    kfree(list->head->prev);
    list->head->prev = NULL;
    list->node_count--;
    return;
}
