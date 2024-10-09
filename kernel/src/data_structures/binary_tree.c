//
// Created by dustyn on 8/31/24.
//
#include "include/definitions.h"
#include "include/data_structures/binary_tree.h"

/* Local Definitions */

void init_red_black_tree(struct binary_tree* tree, void* root, uint64 key);
void init_binary_tree(struct binary_tree* tree, void* root, uint64 key);
uint64 insert_binary_tree(struct binary_tree *tree, void* data, uint64 key);
uint64 insert_red_black_tree(struct binary_tree *tree, void* data, uint64 key);
uint64 remove_binary_tree(struct binary_tree *tree, uint64 key);
uint64 remove_red_black_tree(struct binary_tree *tree, uint64 key);


uint64 init_tree(struct binary_tree *tree,uint64 mode,uint64 flags, void* root, uint64 key) {

    switch (mode) {

    case REGULAR_TREE:
        init_binary_tree(tree,root,key);
        return SUCCESS;

    case RED_BLACK_TREE:
        init_red_black_tree(tree,root,key);
        return SUCCESS;

        default:
            return BAD_TREE_MODE;

    }

}


uint64 insert_tree(struct binary_tree *tree, void* data, uint64 key){
    switch (tree->mode) {
        case REGULAR_TREE:
            return insert_binary_tree(tree,data,key);


        case RED_BLACK_TREE:
            return insert_red_black_tree(tree,data,key);
        default:
            return BAD_TREE_MODE;
    }
}


uint64 remove_tree(struct binary_tree *tree, uint64 key) {
    switch (tree->mode) {
    case REGULAR_TREE:
        return remove_binary_tree(tree,key);
    case RED_BLACK_TREE:
        return remove_red_black_tree(tree,key);
    default:
        return BAD_TREE_MODE;
    }
}

uint64 destroy_tree(struct binary_tree *tree){

}

uint64 insert_binary_tree(struct binary_tree *tree, void* data, uint64 key) {

}

uint64 insert_red_black_tree(struct binary_tree *tree, void* data, uint64 key) {

}

uint64 remove_binary_tree(struct binary_tree *tree, uint64 key) {

}

uint64 remove_red_black_tree(struct binary_tree *tree, uint64 key) {
}


void init_red_black_tree(struct binary_tree *tree, void* root, uint64 key){
    tree->mode = RED_BLACK_TREE;
    tree->root->color = BLACK;
    tree->root->key = key;
    tree->root->data = root;
    tree->root->parent = NULL;
    tree->node_count = 1;
}


void init_binary_tree(struct binary_tree *tree, void* root, uint64 key){
    tree->mode = REGULAR_TREE;
    tree->root->key = key;
    tree->root->data = root;
    tree->root->parent = NULL;
    tree->node_count = 1;
}

