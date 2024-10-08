//
// Created by dustyn on 8/31/24.
//

#pragma once
#include "include/types.h"

#define BLACK 1
#define RED 0

#define REGULAR_TREE 0
#define RED_BLACK 1

struct red_black_tree {
    struct red_black_tree_node* root;
    uint64 node_count;
};

struct red_black_tree_node {
    struct red_black_tree_node *parent;
    struct red_black_tree_node* left;
    struct red_black_tree_node* right;
    void *data;
    uint64 color;
};
