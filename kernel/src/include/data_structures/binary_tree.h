//
// Created by dustyn on 8/31/24.
//

#pragma once
#include "include/types.h"

#define BLACK 1
#define RED 0

#define REGULAR_TREE 0
#define RED_BLACK_TREE 1

struct binary_tree {
    struct binary_tree_node* root;
    uint64 node_count;
};

struct binary_tree_node {
    struct binary_tree_node *parent;
    struct binary_tree_node* left;
    struct binary_tree_node* right;
    void *data;
    uint64 color;
};

void init_tree(struct binary_tree *tree,uint64 mode,uint64 flags);