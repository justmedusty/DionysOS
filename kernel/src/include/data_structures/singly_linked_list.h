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
  struct singly_linked_list_node* tail;
  uint64 node_count;
};

void singly_linked_list_init(struct singly_linked_list_node *head);
void singly_linked_list_insert_tail(struct singly_linked_list_node *head, void *data);
struct singly_linked_list_node*  singly_linked_list_insert_head(struct singly_linked_list_node *old_head, void *data);
void singly_linked_list_remove_tail(struct singly_linked_list_node* head);
struct singly_linked_list_node* singled_linked_list_remove_head(struct singly_linked_list_node* head);