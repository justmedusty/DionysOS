//
// Created by dustyn on 8/31/24.
//
#include "include/definitions.h"
#include "include/data_structures/binary_tree.h"
#include <include/mem/kalloc.h>

/* Local Definitions */

void init_red_black_tree(struct binary_tree* tree, void* root, uint64 key);
void init_binary_tree(struct binary_tree* tree, void* root, uint64 key);
uint64 insert_binary_tree(struct binary_tree* tree, void* data, uint64 key);
uint64 insert_red_black_tree(struct binary_tree* tree, void* data, uint64 key);
uint64 remove_binary_tree(struct binary_tree* tree, uint64 key, void* address);
uint64 remove_red_black_tree(struct binary_tree* tree, uint64 key);


uint64 init_tree(struct binary_tree* tree, uint64 mode, uint64 flags, void* root, uint64 key) {
    switch (mode) {
    case REGULAR_TREE:
        init_binary_tree(tree, root, key);
        return SUCCESS;

    case RED_BLACK_TREE:
        init_red_black_tree(tree, root, key);
        return SUCCESS;

    default:
        return BAD_TREE_MODE;
    }
}


uint64 insert_tree_node(struct binary_tree* tree, void* data, uint64 key) {
    switch (tree->mode) {
    case REGULAR_TREE:
        return insert_binary_tree(tree, data, key);


    case RED_BLACK_TREE:
        return insert_red_black_tree(tree, data, key);
    default:
        return BAD_TREE_MODE;
    }
}


uint64 remove_tree_node(struct binary_tree* tree, uint64 key, void* address) {
    switch (tree->mode) {
    case REGULAR_TREE:
        return remove_binary_tree(tree, key, address);
    case RED_BLACK_TREE:
        return remove_red_black_tree(tree, key);
    default:
        return BAD_TREE_MODE;
    }
}

/* Gonna need to trace this out to ensure it works properly*/
uint64 destroy_tree(struct binary_tree* tree) {
    struct binary_tree_node* current = tree->root;
    struct binary_tree_node* parent = tree->root;

    if (current == NULL) {
        kfree(tree);
        return SUCCESS;
    }
    while (tree->node_count != 0) {
        /* Leaf node ? */
        if (current->left == NULL && current->right == NULL) {
            parent = current->parent;

            if (current->parent->left == current) {
                current->parent->left = NULL;
            }
            else {
                current->parent->right = NULL;
            }

            kfree(current);
            current = parent;
            tree->node_count--;
        }
        if (current->left != NULL) {
            current = current->left;
            parent = current->parent;
            continue;
        }
        if (current->right != NULL) {
            current = current->right;
            parent = current->parent;
            continue;
        }
    }

    kfree(tree);
}

uint64 insert_binary_tree(struct binary_tree* tree, void* data, uint64 key) {
    struct binary_tree_node* new_node = kalloc(sizeof(struct binary_tree_node));
    new_node->count = 1;
    new_node->data[new_node->count++ - 1] = data;
    new_node->key = key;


    struct binary_tree_node* current = tree->root;

    if (current == NULL) {
        tree->root = new_node;
        tree->node_count++;
        return SUCCESS;
    }


    while (1) {
        // Sanity checks above and below mean that there shouldn't be any null nodes showing up here
        if (key == current->key) {
            if (current->count == MAX_DATA_PER_NODE) {
                return INSERTION_ERROR;
            }
            current->data[current->count++ - 1] = data;
            kfree(new_node);
            return SUCCESS;
        }

        if (key < current->key) {
            if (current->left == NULL) {
                current->left = new_node;
                new_node->parent = current;
                tree->node_count++;
                return SUCCESS;
            }

            current = current->left;
        }
        else {
            if (current->right == NULL) {
                current->right = new_node;
                new_node->parent = current;
                tree->node_count++;
                return SUCCESS;
            }
            current = current->right;
        }
    }
}

void* lookup_tree(struct binary_tree* tree, uint64 key) {
    struct binary_tree_node* current = tree->root;

    if (current == NULL) {
        return NULL;
    }

    while (1) {
        // Sanity checks below mean that there shouldn't be any null nodes showing up here
        if (key == current->key) {
            return current->data[current->count - 1];
        }

        if (key < current->key) {
            if (current->left == NULL) {
                return NULL;
            }

            current = current->left;
        }
        else {
            if (current->right == NULL) {
                return NULL;
            }

            current = current->right;
        }
    }
}

uint64 insert_red_black_tree(struct binary_tree* tree, void* data, uint64 key) {
}

uint64 remove_binary_tree(struct binary_tree* tree, uint64 key, void* address/* This is required because there may be many of the same value so address needed to be passed as well */) {
    struct binary_tree_node* current = tree->root;
    struct binary_tree_node* parent = tree->root;

    if (current == NULL) {
        return VALUE_NOT_FOUND;
    }

    if (key == tree->root->key) {
        //handle root
    }


    uint8 depth = 0; /* Not sure if I am going to use this yet */
    while (1) {
        // Sanity checks below mean that there shouldn't be any null nodes showing up here
        if (key == current->key) {
            /*
             *  Handle Leaf case
             */

            if (current->count < 1) {
                for (int i = 0; i < current->count; i++) {
                    if (current->data[i] == address) {
                        current->count--;
                        current->data[i] = NULL;
                        return SUCCESS;
                    }
                }
            }

            if (current->left == NULL && current->right == NULL) {
                parent = current->parent;

                if (parent->left == current) {
                    parent->left = NULL;
                }
                else {
                    parent->right = NULL;
                }
                kfree(current);
                return SUCCESS;
            }


            /*
             * Handle right child is non-null while left child is
             */
            if (current->left == NULL) {

                current->parent->right = current->right;
                kfree(current);
                return SUCCESS;
            }
            /*
             *  Handle left child is non-null while right child is
             */
            if (current->right == NULL) {
                current->parent->left = current->left;
                kfree(current);
                return SUCCESS;
            }

            /*
             *
             *  Handle neither child is non-null
             *  Bring the right sub-tree min value up
             */
            parent = current->parent;
            struct binary_tree_node* right_min;
            right_min = current->right;

            while(right_min->left != NULL && right_min->left->right != NULL) {
                right_min = right_min->left;
            }
            parent->left = right_min;
        }

        if (key < current->key) {
            if (current->left == NULL) {
                return VALUE_NOT_FOUND;
            }

            current = current->left;
            depth++;
        }
        else {
            if (current->right == NULL) {
                return VALUE_NOT_FOUND;
            }
            current = current->right;
            depth++;
        }
    }
}


uint64 remove_red_black_tree(struct binary_tree* tree, uint64 key) {
}


void init_red_black_tree(struct binary_tree* tree, void* root, uint64 key) {
    tree->mode = RED_BLACK_TREE;
    tree->root->color = BLACK;
    tree->root->key = key;
    tree->root->count = 0;
    tree->root->data[tree->root->count++] = root;
    tree->root->parent = NULL;
    tree->node_count = 1;
}


void init_binary_tree(struct binary_tree* tree, void* root, uint64 key) {
    tree->mode = REGULAR_TREE;
    tree->root->key = key;
    tree->root->count = 0;
    tree->root->data[tree->root->count++] = root;
    tree->root->parent = NULL;
    tree->node_count = 1;
}

