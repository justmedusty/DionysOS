//
// Created by dustyn on 8/31/24.
//

#pragma once
#include "singly_linked_list.h"
#include "spinlock.h"
#include "include/definitions/types.h"

#define BLACK_NODE 0xF
#define RED_NODE 0xE

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
    uint64_t node_count;
    uint64_t mode;
    uint64_t flags; /* Unused for now */
};

struct binary_tree_node {
    struct binary_tree_node* parent;
    struct binary_tree_node* left;
    struct binary_tree_node* right;
    uint32_t key; /* This is a duplicate value but I have to put it here to allow void pointers otherwise I would be limited by type */
    struct singly_linked_list data;
    uint16_t color; /* Only for RB tree */
    uint16_t flags;
    uint8_t index;
};

uint64_t init_tree(struct binary_tree* tree, uint64_t mode, uint64_t flags);
uint64_t insert_tree_node(struct binary_tree* tree, void* data, uint64_t key);
uint64_t remove_tree_node(struct binary_tree *tree, uint64_t key,void *address,struct binary_tree_node *node /* Optional */);
uint64_t destroy_tree(struct binary_tree* tree);
void* lookup_tree(struct binary_tree* tree, uint64_t key,uint8_t remove);

/*
 * The PMM tree ops cannot have locked memory functions since it will cause a deadlock,
 * these macros are here to ensure that they use the unlocked functions where needed
 */
#define NODE_ALLOC(name) struct binary_tree_node* name; \
if (IS_PMM_TREE) {\
    name = pmm_node_alloc();\
}else {\
    name = node_alloc();\
}\

#define NODE_FREE(name) \
if (IS_PMM_TREE) {\
pmm_node_free(name);\
}else {\
node_free(name);\
}\

#define IS_PMM_TREE (tree == &buddy_free_list_zone[0])
extern struct binary_tree buddy_free_list_zone[PHYS_ZONE_COUNT];