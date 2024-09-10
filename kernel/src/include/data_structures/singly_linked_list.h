//
// Created by dustyn on 6/21/24.
//
#pragma once

struct singly_linked_list_node{
  void* data;
  struct singly_linked_list_node* next;
};


void singly_linked_list_init(struct singly_linked_list_node *head);
void singly_linked_list_insert_tail(struct singly_linked_list_node *head, void *data);
struct singly_linked_list_node*  singly_linked_list_insert_head(struct singly_linked_list_node *old_head, void *data);
