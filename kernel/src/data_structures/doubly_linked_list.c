//
// Created by dustyn on 9/11/24.
//

#include "include/data_structures/doubly_linked_list.h"
#include "include/types.h"
#include <include/kalloc.h>
#include "include/definitions.h"

/*
 * I will add head and tail fields into the lists should there be a possibility of massive lists but for now will just do a linear traversal approach.
 */

void doubly_linked_list_init(struct doubly_linked_list *list) {
    list->head = NULL;
    list->tail = NULL;
    list->node_count = 0;
}

void doubly_linked_list_insert_tail(struct doubly_linked_list* list, void* data) {

    struct doubly_linked_list_node* new_node = kalloc(sizeof(struct doubly_linked_list_node));;
    new_node->data = data;

    if(list->head == NULL && list->tail == NULL) {
      list->head = new_node;
      list->tail = new_node;
      list->node_count++;
      return;
    }

     if(list->tail == list->head) {
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

    if(list->head == NULL && list->tail == NULL) {
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

void doubly_linked_list_remove_tail(struct doubly_linked_list_node* head) {
    struct doubly_linked_list_node* node = head;
    struct doubly_linked_list_node* new_tail = NULL;

    if(node->next == NULL) {
        kfree(node);
        return;
    }

    while (node->next != NULL) {
        if (node->next->next == NULL) {
            new_tail = node;
            node = node->next;
            break;
        }
        node = node->next;
    }
    new_tail->next = NULL;
    kfree(node);
}

void doubly_linked_list_remove_head(struct doubly_linked_list_node* head) {

}