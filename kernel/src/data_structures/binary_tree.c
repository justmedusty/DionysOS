//
// Created by dustyn on 8/31/24.
//
#include "include/definitions.h"
#include "include/data_structures/binary_tree.h"

#include <include/mem/kalloc.h>

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


uint64 insert_tree_node(struct binary_tree *tree, void* data, uint64 key){
    switch (tree->mode) {
        case REGULAR_TREE:
            return insert_binary_tree(tree,data,key);


        case RED_BLACK_TREE:
            return insert_red_black_tree(tree,data,key);
        default:
            return BAD_TREE_MODE;
    }
}


uint64 remove_tree_node(struct binary_tree *tree, uint64 key) {
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

    struct binary_tree_node* new_node = kalloc(sizeof(struct binary_tree_node));
    new_node->count = 0;
    new_node->data[new_node->count++]= data;
    new_node->key = key;


    struct binary_tree_node *current = tree->root;

    if(current == NULL) {
        tree->root = new_node;
        return SUCCESS;
    }


    while(1) {

        if(current != NULL && key == current->key) {
            if(current->count == MAX_DATA) {
                return INSERTION_ERROR;
            }
            current->data[current->count++] = data;
            return SUCCESS;
        }



        if(key <= current->key) {
            if(current->left == NULL) {
                current->left = new_node;
                new_node->parent = current;
                tree->node_count++;
                return SUCCESS;
            }

            current = current->left;
        }else {

            if(current->right == NULL) {
                current->right = new_node;
                new_node->parent = current;
                tree->node_count++;
                return SUCCESS;
            }
            current = current->right;
        }

    }



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
    tree->root->count = 0;
    tree->root->data[tree->root->count++] = root;
    tree->root->parent = NULL;
    tree->node_count = 1;
}


void init_binary_tree(struct binary_tree *tree, void* root, uint64 key){
    tree->mode = REGULAR_TREE;
    tree->root->key = key;
    tree->root->count = 0;
    tree->root->data[tree->root->count++] = root;
    tree->root->parent = NULL;
    tree->node_count = 1;
}

