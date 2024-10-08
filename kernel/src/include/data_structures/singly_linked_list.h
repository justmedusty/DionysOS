//
// Created by dustyn on 6/21/24.
//
#pragma once
#include <include/types.h>

struct singly_linked_list_node{
  void* data;
  struct singly_linked_list_node* next;
};

struct singly_linked_list {
  struct singly_linked_list_node* head;
  //I will keep tail now anyway but you cannot really use it to skip the line since you cant update its predecessors pointer without a walk. It may be useful in the case we need to insert something at the end however.
  struct singly_linked_list_node* tail;
  uint64 node_count;
};


void singly_linked_list_init(struct singly_linked_list *list);
void singly_linked_list_insert_tail(struct singly_linked_list *list, void *data);
void singly_linked_list_insert_head(struct singly_linked_list *list, void *data);
void singly_linked_list_remove_tail(struct singly_linked_list *list);
void singly_linked_list_remove_head(struct singly_linked_list *list);