//
// Created by dustyn on 6/21/24.
//
#ifndef SINGLY_LINKED_LIST_H
#define SINGLY_LINKED_LIST_H
#include "spinlock.h"

#define NODE_NOT_FOUND 1

#define SINGLY_LINKED_LIST_NODE_STATIC_POOL_SIZE 80000UL
#define STATIC_POOL_NODE 4
#define STATIC_POOL_FREE_NODE 8

#define LIST_FLAG_FREE_NODES 16

struct singly_linked_list_node{
  void* data;
  struct singly_linked_list_node* next;
  uint64_t flags;
  uint64_t reserved;
};

struct singly_linked_list {
  struct spinlock lock;
  struct singly_linked_list_node* head;
  //I will keep tail now anyway but you cannot really use it to skip the line since you cant update its predecessors pointer without a walk. It may be useful in the case we need to insert something at the end however.
  struct singly_linked_list_node* tail;
  uint64_t node_count;
  uint64_t flags;
};


void singly_linked_list_init(struct singly_linked_list *list,uint64_t flags);
void singly_linked_list_insert_tail(struct singly_linked_list *list, void *data);
void singly_linked_list_insert_head(struct singly_linked_list *list, void *data);
void *singly_linked_list_remove_tail(struct singly_linked_list *list);
void *singly_linked_list_remove_head(struct singly_linked_list *list);
uint64_t singly_linked_list_remove_node_by_address(struct singly_linked_list* list, void* data);
void singly_linked_list_destroy(struct singly_linked_list *list); // NOTE if the nodes hold allocated memory it will be leaked! don't call this if there's allocated memory and you only have the one reference!
#endif