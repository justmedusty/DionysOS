//
// Created by dustyn on 8/31/24.
//

#pragma once
#include "singly_linked_list.h"
#include "spinlock.h"
#include "include/types.h"

#define BLACK 0xF
#define RED 0xE

#define REGULAR_TREE 1
#define RED_BLACK_TREE 2

#define BINARY_TREE_NODE_STATIC_POOL_SIZE 24
#define BINARY_TREE_NODE_STATIC_POOL 1
#define BINARY_TREE_NODE_FREE 2

/* Other responses */
#define BAD_TREE_MODE 0xF1F
#define INSERTION_ERROR 0x1234

#define SUCCESS 0
#define EMPTY_TREE 1
#define VALUE_NOT_FOUND 2

#define REMOVE_FROM_TREE 1
#define DO_NOT_REMOVE_FROM_TREE 2

#define EMPTY_TREE_CAST (void *) 1

struct binary_tree {
    struct spinlock lock; /* Optional */
    struct binary_tree_node* root;
    uint64 node_count;
    uint64 mode;
    uint64 flags; /* Unused for now */
};

struct binary_tree_node {
    struct binary_tree_node* parent;
    struct binary_tree_node* left;
    struct binary_tree_node* right;
    uint64 key; /* This is a duplicate value but I have to put it here to allow void pointers otherwise I would be limited by type */
    struct singly_linked_list data;
    uint16 color; /* Only for RB tree */
    uint16 flags;
    uint8 index;
};

uint64 init_tree(struct binary_tree* tree, uint64 mode, uint64 flags);
uint64 insert_tree_node(struct binary_tree* tree, void* data, uint64 key);
uint64 remove_tree_node(struct binary_tree *tree, uint64 key,void *address,struct binary_tree_node *node /* Optional */);
uint64 destroy_tree(struct binary_tree* tree);
void* lookup_tree(struct binary_tree* tree, uint64 key,uint8 remove);
