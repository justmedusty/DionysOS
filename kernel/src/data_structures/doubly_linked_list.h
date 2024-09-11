//
// Created by dustyn on 9/11/24.
//

#ifndef DOUBLY_LINKED_LIST_H
#define DOUBLY_LINKED_LIST_H


struct doubly_linked_list_node{
    void* data;
    struct doubly_linked_list_node* next;
    struct doubly_linked_list_node* prev;
};


#endif //DOUBLY_LINKED_LIST_H
