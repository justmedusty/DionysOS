//
// Created by dustyn on 8/31/24.
//

#pragma once
#include "include/types.h"

#define BLACK 0xF
#define RED 0xE

#define REGULAR_TREE 1
#define RED_BLACK_TREE 2

#define MAX_DATA_PER_NODE 20 /* Arbitrary but should be enough*/

/* Other responses */
#define BAD_TREE_MODE 0xF1F
#define INSERTION_ERROR 0x1234
#define SUCCESS 0
#define EMPTY_TREE 1
#define VALUE_NOT_FOUND 2


struct binary_tree {
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
    void* data[4]; /* I may want to reign this in with a flag explicitly allowing this behavior , sticking a pin in it*/
    /* Since for things like buddy, there may be many blocks with the same order I will allow nodes to store many values just in case. 4 is arbitrary for now I may make it dynamic later*/
    uint64 count;
    uint64 color; /* Only for RB tree */
};

uint64 init_tree(struct binary_tree* tree, uint64 mode, uint64 flags, void* data, uint64 key);
uint64 insert_tree_node(struct binary_tree* tree, void* data, uint64 key);
uint64 remove_tree_node(struct binary_tree *tree, uint64 key,void *address);
uint64 destroy_tree(struct binary_tree* tree);

