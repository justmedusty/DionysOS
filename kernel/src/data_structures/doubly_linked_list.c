//
// Created by dustyn on 9/11/24.
//

#include "include/data_structures/doubly_linked_list.h"

#include "include/definitions/string.h"

#include "include/definitions/types.h"
#include <include/memory/kalloc.h>
#include "include/definitions/definitions.h"

/*
 * A simple doubly linked list implementation which can be used anywhere in the kernel later on.
* I added a simple node_count since I think this will be useful to have at some point or other.
 */

void doubly_linked_list_init(struct doubly_linked_list *list) {
    initlock(&list->lock,DOUBLY_LINKED_LIST_LOCK);
    list->head = NULL;
    list->tail = NULL;
    list->node_count = 0;
}

void doubly_linked_list_insert_tail(struct doubly_linked_list* list, void* data) {
    acquire_spinlock(&list->lock);
    struct doubly_linked_list_node* new_node =kmalloc(sizeof(struct doubly_linked_list_node));;
    new_node->data = data;

    if(list->node_count == 0) {
      list->head = new_node;
      list->tail = NULL;
      list->node_count++;
        release_spinlock(&list->lock);
      return;
    }

     if(list->node_count == 1) {
       list->tail = new_node;
       list->head->next = new_node;
       list->tail->prev = list->head;
       list->node_count++;
         release_spinlock(&list->lock);
       return;
     }

     list->tail->next = new_node;
     new_node->prev = list->tail;
     list->tail = new_node;
     list->node_count++;
    release_spinlock(&list->lock);

}

void doubly_linked_list_insert_head(struct doubly_linked_list* list, void* data) {
    acquire_spinlock(&list->lock);
    struct doubly_linked_list_node* new_head =kmalloc(sizeof(struct doubly_linked_list_node));
    new_head->data = data;

    if(list->node_count == 0) {
      list->head = new_head;
        new_head->next = NULL;
        new_head->prev = NULL;
      list->tail = NULL;
      list->node_count++;
        release_spinlock(&list->lock);
      return;
    }

    new_head->next = list->head;
    list->head->prev = new_head;
    list->head = new_head;
    new_head->prev = NULL;
    list->node_count++;
    release_spinlock(&list->lock);
    return;
}

void doubly_linked_list_remove_tail(struct doubly_linked_list* list) {
    acquire_spinlock(&list->lock);
    if(list->node_count == 0) {
        return;
    }

    if(list->node_count == 1) {
      kfree(list->head);
      list->head = NULL;
      list->tail = NULL;
      list->node_count--;
        release_spinlock(&list->lock);
      return;
    }

     list->tail = list->tail->prev;
     kfree(list->tail->next);
     list->tail->next = NULL;
     list->node_count--;
    release_spinlock(&list->lock);
     return;
}

void doubly_linked_list_remove_head(struct doubly_linked_list* list) {
    acquire_spinlock(&list->lock);
    if(list->node_count == 0) {
        release_spinlock(&list->lock);
        return;
    }

    if(list->node_count == 1) {
        kfree(list->head);
        list->head = NULL;
        list->tail = NULL;
        list->node_count--;
        release_spinlock(&list->lock);
        return;
    }

    list->head = list->head->next;
    kfree(list->head->prev);
    list->head->prev = NULL;
    list->node_count--;
    release_spinlock(&list->lock);
    return;
}

void doubly_linked_list_remove_node_by_address(struct doubly_linked_list *list,struct doubly_linked_list_node* node) {
    acquire_spinlock(&list->lock);
    if (node->next == NULL && node->prev == NULL) {
        list->head = NULL;
        list->tail = NULL;
        list->node_count--;
        release_spinlock(&list->lock);
        return;
    }
    if (node->next == NULL) {
        list->tail = node->prev;
        list->tail->next = NULL;
        list->node_count--;
        release_spinlock(&list->lock);
        return;
    }
    if (node->prev == NULL) {
        list->head = node->next;
        list->head->prev = NULL;
        list->node_count--;
        release_spinlock(&list->lock);
        return;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    list->node_count--;
    release_spinlock(&list->lock);

}

void doubly_linked_list_remove_node_by_data_address(struct doubly_linked_list *list, const void *data) {
    acquire_spinlock(&list->lock);
    const struct doubly_linked_list_node* node = list->head;

    while (node && node->data != data) {
        node = node->next;
    }

    if (node == NULL) {
        release_spinlock(&list->lock);
        return;
    }

    if (node->next == NULL && node->prev == NULL) {
        list->head = NULL;
        list->tail = NULL;
        list->node_count--;
        release_spinlock(&list->lock);
        return;
    }

    if (node->next == NULL) {
        list->tail = node->prev;
        list->tail->next = NULL;
        list->node_count--;
        release_spinlock(&list->lock);
        return;
    }

    if (node->prev == NULL) {
        list->head = node->next;
        list->head->prev = NULL;
        list->node_count--;
        release_spinlock(&list->lock);
        return;
    }

    node->prev->next = node->next;
    node->next->prev = node->prev;
    list->node_count--;
    release_spinlock(&list->lock);
}


void doubly_linked_list_destroy(struct doubly_linked_list* list,bool free_data) {
    if (list == NULL) return;
    acquire_spinlock(&list->lock);
    struct doubly_linked_list_node* current = list->head;
    while (current != NULL) {
        struct doubly_linked_list_node* next = current->next;
        if(free_data){
            kfree(current->data); // this is really iffy since we do not know what data is so I will put it behind a bool
        }
        kfree(current);
        current = next;
    }
    release_spinlock(&list->lock);
    kfree(list);
}
