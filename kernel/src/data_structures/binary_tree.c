//
// Created by dustyn on 8/31/24.
//
#include "include/definitions.h"
#include "include/data_structures/binary_tree.h"

#include <string.h>
#include <include/arch/arch_cpu.h>
#include <include/drivers/uart.h>
#include <include/mem/kalloc.h>

/* Local Definitions */


static struct binary_tree_node tree_node_static_pool[BINARY_TREE_NODE_STATIC_POOL_SIZE];
static uint8 pool_full = 0;
static int32 last_freed = -1; /* These two fields are just simple safeguards to prevent linear lookups when possible */

void init_red_black_tree(struct binary_tree* tree);
void init_binary_tree(struct binary_tree* tree);
uint64 insert_binary_tree(struct binary_tree* tree, void* data, uint64 key);
uint64 insert_red_black_tree(struct binary_tree* tree, void* data, uint64 key);
uint64 remove_binary_tree(struct binary_tree* tree, uint64 key, void* address, struct binary_tree_node* node);
uint64 remove_red_black_tree(struct binary_tree* tree, uint64 key);

static uint8 static_pool_init = 0;

static struct binary_tree_node* node_alloc() {
    if (pool_full) {
        panic("Binary tree pool is full");
        return kalloc(sizeof(struct binary_tree_node));
    }

    for (uint64 i = 0; i < BINARY_TREE_NODE_STATIC_POOL_SIZE; i++) {
        if (tree_node_static_pool[i].flags & BINARY_TREE_NODE_FREE) {
            tree_node_static_pool[i].flags &= ~BINARY_TREE_NODE_FREE;
            tree_node_static_pool[i].index = i;
            return &tree_node_static_pool[i];
        }
    }
    pool_full = 1;
    return NULL;
}

static void node_free(struct binary_tree_node* node) {
    if (!(node->flags & BINARY_TREE_NODE_STATIC_POOL)) {
        kfree(node);
        return;
    }

    if (node->right != NULL || node->left != NULL) {
        serial_printf("here");
    }
    if (node->parent != NULL) {
        if (node->parent->right == node) {
            node->parent->right = NULL;
        }
        else {
            node->parent->left = NULL;
        }
    }
    node->flags |= BINARY_TREE_NODE_FREE;
    node->parent = NULL;
    node->left = NULL;
    node->right = NULL;
    node->key = 0xFF;
    if (pool_full) {
        pool_full = 0;
    }
}

uint64 init_tree(struct binary_tree* tree, uint64 mode, uint64 flags) {
    if (static_pool_init == 0) {
        for (uint64 i = 0; i < BINARY_TREE_NODE_STATIC_POOL_SIZE; i++) {
            tree_node_static_pool[i].flags = BINARY_TREE_NODE_STATIC_POOL | BINARY_TREE_NODE_FREE;
        }
    }
    switch (mode) {
    case REGULAR_TREE:
        init_binary_tree(tree);
        return SUCCESS;

    case RED_BLACK_TREE:
        init_red_black_tree(tree);
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


uint64 remove_tree_node(struct binary_tree* tree, uint64 key, void* address,
                        struct binary_tree_node* node /* Optional */) {
    switch (tree->mode) {
    case REGULAR_TREE:
        return remove_binary_tree(tree, key, address, node);
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

            node_free(current);
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
    return SUCCESS;
}

uint64 insert_binary_tree(struct binary_tree* tree, void* data, uint64 key) {
    struct binary_tree_node* current = tree->root;

    if (current == NULL) {
        struct binary_tree_node* new_node = node_alloc();
        singly_linked_list_init(&new_node->data);
        singly_linked_list_insert_head(&new_node->data, data);
        new_node->key = key;
        tree->root = new_node;
        tree->root->parent = NULL;
        tree->node_count++;
        return SUCCESS;
    }

    while (1) {
        // Sanity checks above and below mean that there shouldn't be any null nodes showing up here
        if (key == current->key) {
            singly_linked_list_insert_tail(&current->data, data);
            return SUCCESS;
        }

        if (key < current->key) {
            if (current->left == NULL) {
                struct binary_tree_node* new_node = node_alloc();
                singly_linked_list_init(&new_node->data);
                singly_linked_list_insert_head(&new_node->data, data);
                new_node->key = key;
                current->left = new_node;
                new_node->parent = current;
                if (current == new_node) {
                    serial_printf(
                        "[ERROR] Binary tree newly allocated node is same address as current node in tree!\n");
                }
                if (new_node->parent == new_node) {
                    serial_printf("[ERROR] Binary tree node already exists\n");
                }

                tree->node_count++;
                return SUCCESS;
            }
            current = current->left;
        }
        else {
            if (current->right == NULL) {
                struct binary_tree_node* new_node = node_alloc();
                singly_linked_list_init(&new_node->data);
                singly_linked_list_insert_head(&new_node->data, data);
                new_node->key = key;
                current->right = new_node;
                new_node->parent = current;
                tree->node_count++;
                if (current == new_node) {
                    serial_printf(
                        "[ERROR] Binary tree newly allocated node is same address as current node in tree!\n");
                }
                if (new_node->parent == new_node) {
                    serial_printf("[ERROR] Binary tree node already exists\n");
                }

                return SUCCESS;
            }
            current = current->right;
        }
    }
}


void* lookup_tree(struct binary_tree* tree, uint64 key, uint8 remove /* Flag to remove it from the tree*/) {
    if ((tree_node_static_pool[2].parent != NULL && tree_node_static_pool[3].parent != NULL) && tree_node_static_pool[2]
        .parent == tree_node_static_pool[3].parent) {
        panic("[ERROR] Fucked up tree\n");
    }
    struct binary_tree_node* current = tree->root;
    if (current == NULL) {
        return NULL; /* Indication of empty tree */
    }

    while (1) {
        // Sanity checks below mean that there shouldn't be any null nodes showing up here
        if (key == current->key) {
            void* return_value = 0;
            if (remove) {
                //TODO investigate possible race condition here.
                if (current->parent && current->parent->key == 0xFF) {
                    panic("[ERROR] Binary tree node doesn't exists\n");
                }
                return_value = current->data.head->data;
                remove_tree_node(tree, key, current->data.head, current);
            }

            return return_value;
        }

        if (key < current->key) {
            if (current->left == current) {
                current->left = NULL;
            }
            if (current->left == NULL) {
                return NULL;
            }

            current = current->left;
        }
        else {
            if (current->right == current) {
                current->right = NULL;
            }
            if (current->right == NULL) {
                return NULL;
            }

            current = current->right;
        }
    }
}

uint64 insert_red_black_tree(struct binary_tree* tree, void* data, uint64 key) {
}

uint64 remove_binary_tree(struct binary_tree* tree, uint64 key, void* address,
                          struct binary_tree_node* node /* Optional */
                          /* This is required because there may be many of the same value so address needed to be passed as well */) {
    acquire_spinlock(&tree->lock);

    struct binary_tree_node* current = tree->root;
    struct binary_tree_node* parent = tree->root;

    if (node != NULL) {
        current = node;
        parent = node->parent;
    }

    if (current == NULL) {
        release_spinlock(&tree->lock);
        return VALUE_NOT_FOUND;
    }

    if (key == tree->root->key) {
        singly_linked_list_remove_node_by_address(&current->data, address);
        release_spinlock(&tree->lock);
        return SUCCESS;
    }
    uint8 depth = 0; /* Not sure if I am going to use this yet */
    while (1) {
        // Sanity checks below mean that there shouldn't be any null nodes showing up here
        if (key == current->key) {
            /*
             *  Handle Leaf case
             */

            /*
             *  Handle case where it is in a non-empty bucket
             */
            if (current->data.node_count > 1) {
                if (singly_linked_list_remove_node_by_address(&current->data, address) == SUCCESS) {
                    release_spinlock(&tree->lock);
                    return SUCCESS;
                }
                release_spinlock(&tree->lock);
                return VALUE_NOT_FOUND;
            }


            if (current->left == NULL && current->right == NULL) {
                //TODO might need to put protection for root node so no deref
                if(current->parent->left == current) {
                    current->parent->left = NULL;
                }else {
                    current->parent->right = NULL;
                }
                node_free(current);
                tree->node_count--;
                release_spinlock(&tree->lock);
                return SUCCESS;
            }


            /*
             * Handle right child is non-null while left child is
             */
            if (current->left == NULL) {
                if (current->parent->key == 256) {
                    panic("[ERROR] LEFT NULL KEY 256\n");
                }

                if(current->parent->right == current) {
                    current->parent->right->parent = current->parent;
                }

                if(current->right) {
                    current->right->parent = current->parent;
                }

                node_free(current);
                tree->node_count--;
                release_spinlock(&tree->lock);
                return SUCCESS;
            }
            /*
             *  Handle left child is non-null while right child is
             */

            if (current->right == NULL) {
                if (current->parent->key == 256) {
                    panic("[ERROR] RIGHT NULL KEY 256\n");
                }

                if(current->parent->left) {
                    current->parent->left->parent = current->parent;
                }

                if(current->left) {
                    current->left->parent = current->parent;
                }

                if(current->parent->left == current) {
                    current->parent->left->parent = current->parent;
                }
                node_free(current);
                tree->node_count--;
                release_spinlock(&tree->lock);
                return SUCCESS;
            }
            /*
             *
             *  Handle neither child is non-null
             *  Bring the right sub-tree min value up (ie all lefts until a final right)p
             */
            panic("here");
            parent = current->parent;
            struct binary_tree_node* right_min = current;

            if (right_min->left == right_min || right_min->parent == right_min) {
                panic("Circular Reference");
            }
            while (right_min->left != NULL) {
                right_min = right_min->left;
            }

            /* I dont think this should happen but I am putting it here just in case there is a right chain, I may remove this later, just me being paranoid  */
            while (right_min->right != NULL) {
                right_min = right_min->right;
            }


            parent->left = right_min;
            right_min->right = current->right;
            right_min->left = current->left;

            if (right_min->left == right_min || right_min->parent == right_min) {
                panic("Circular Reference NUMBA 2");
            }
            node_free(current);
            tree->node_count--;
            release_spinlock(&tree->lock);
            return SUCCESS;
        }

        if (key < current->key) {
            if (current->left == NULL) {
                release_spinlock(&tree->lock);
                return VALUE_NOT_FOUND;
            }

            current = current->left;
            depth++;
        }
        else {
            if (current->right == NULL) {
                release_spinlock(&tree->lock);
                return VALUE_NOT_FOUND;
            }
            current = current->right;
            depth++;
        }
    }
}


uint64 remove_red_black_tree(struct binary_tree* tree, uint64 key) {
}


void init_red_black_tree(struct binary_tree* tree) {
    tree->mode = RED_BLACK_TREE;
    tree->root->color = BLACK;
    tree->node_count = 1;
    initlock(&tree->lock, BTREE_LOCK);
}


void init_binary_tree(struct binary_tree* tree) {
    tree->mode = REGULAR_TREE;
    tree->node_count = 0;
    tree->root = NULL;
    initlock(&tree->lock, BTREE_LOCK);
}

