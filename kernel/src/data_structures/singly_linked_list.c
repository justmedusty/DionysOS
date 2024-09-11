//
// Created by dustyn on 6/21/24.
//

#include "include/data_structures/singly_linked_list.h"
#include "include/types.h"
#include <include/kalloc.h>

#include "include/definitions.h"

/*
 *  Generic Implementation of a singly linked list with some functions to operate on them.
 *  I will leave out things like sorts since I am trying to make this as generic as possible and I can't sort without knowing the type the data pointer leads to.
 */

void singly_linked_list_init(struct singly_linked_list_node* head) {
    head->next = NULL;
    head->data = NULL;
}

void singly_linked_list_insert_tail(struct singly_linked_list_node* head, void* data) {
    struct singly_linked_list_node* node = head;

    while (node->next != NULL) {
        node = node->next;
    }

    node->next = kalloc(sizeof(struct singly_linked_list_node));
    node->next->data = data;
}

struct singly_linked_list_node* singly_linked_list_insert_head(struct singly_linked_list_node* old_head, void* data) {
    struct singly_linked_list_node* new_head = kalloc(sizeof(struct singly_linked_list_node));
    new_head->data = data;
    new_head->next = old_head;
    return new_head;
}


void singly_linked_list_remove_tail(struct singly_linked_list_node* head) {
    struct singly_linked_list_node* node = head;
    struct singly_linked_list_node* new_tail = NULL;

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

struct singly_linked_list_node* singly_linked_list_remove_head(struct singly_linked_list_node* head) {
    if (head->next == NULL) {
        kfree(head);
        return NULL;
    }

    struct singly_linked_list_node* new_head = head->next;
    kfree(head);
    return new_head;
}

/*
 *  I will set up tests to test my generic data structure interaction functions and just panic the result. Will only be run once manually and then won't be called until a modification is made during the development process.
 */

void linked_list_test() {
    struct singly_linked_list_node* head = kalloc(sizeof(struct singly_linked_list_node));
    struct singly_linked_list_node** nodes;

    for (int i = 0; i < 10; i++) {
        singly_linked_list_insert_tail(head,nodes[i]);
    }

    for (int i = 0; i < 10; i++) {
        //if(head->next)
    }





}