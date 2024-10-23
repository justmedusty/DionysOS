//
// Created by dustyn on 6/21/24.
//
#pragma once
#include <include/types.h>

#define SUCCESS 0
#define NODE_NOT_FOUND 1

#define SINGLY_LINKED_LIST_NODE_STATIC_POOL_SIZE 10000
#define STATIC_POOL_NODE 4
#define STATIC_POOL_FREE_NODE 8

struct singly_linked_list_node{
  void* data;
  struct singly_linked_list_node* next;
  uint64 flags;
  uint64 reserved;
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
void *singly_linked_list_remove_tail(struct singly_linked_list *list);
void *singly_linked_list_remove_head(struct singly_linked_list *list);
uint64 singly_linked_list_remove_node_by_address(struct singly_linked_list* list, void* data);