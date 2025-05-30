//
// Created by dustyn on 8/31/24.
//
#include "include/definitions/definitions.h"
#include "include/data_structures/binary_tree.h"

#include "include/definitions/string.h"
#include <include/architecture/arch_cpu.h>
#include <include/drivers/serial/uart.h>
#include <include/memory/kmalloc.h>

/* Local Definitions */


static struct binary_tree_node tree_node_static_pool[BINARY_TREE_NODE_STATIC_POOL_SIZE];
static uint8_t pool_full = 0;
static int32_t last_freed = -1;
/* These two fields are just simple safeguards to prevent linear lookups when possible */

void init_red_black_tree(struct binary_tree *tree);

void init_binary_tree(struct binary_tree *tree);

uint64_t insert_binary_tree(struct binary_tree *tree, void *data, uint64_t key);

uint64_t insert_red_black_tree(struct binary_tree *tree, void *data, uint64_t key);

uint64_t remove_binary_tree(struct binary_tree *tree, uint64_t key, void *address, struct binary_tree_node *node);

uint64_t remove_red_black_tree(struct binary_tree *tree, uint64_t key);

static uint8_t static_pool_init = 0;

/*
 * allocate a binary tree node, check if the static pool is free.
 * If any are , allocate from there
 * Else invoke kalloc and return the new ponter
 */
static struct binary_tree_node *node_alloc() {
    if (pool_full) {
        return kmalloc(sizeof(struct binary_tree_node));
    }

    for (uint64_t i = 0; i < BINARY_TREE_NODE_STATIC_POOL_SIZE; i++) {
        if (tree_node_static_pool[i].flags & BINARY_TREE_NODE_FREE) {
            tree_node_static_pool[i].flags &= ~BINARY_TREE_NODE_FREE;
            tree_node_static_pool[i].index = i;
            return &tree_node_static_pool[i];
        }
    }
    pool_full = 1;
    return kmalloc(sizeof(struct binary_tree_node));
}

/*
 * Free a node, just invoke _kfree( it not part of the static pool,
 * other wise clear it and return it to the pool
 */
static void node_free(struct binary_tree_node *node) {
    if (!(node->flags & BINARY_TREE_NODE_STATIC_POOL)) {
        kfree(node);
        return;
    }

    if (node->parent != NULL) {
        if (node->parent->right == node) {
            node->parent->right = NULL;
        } else {
            node->parent->left = NULL;
        }
    }

    node->flags |= BINARY_TREE_NODE_FREE;
    node->parent = NULL;
    node->left = NULL;
    node->right = NULL;
    node->key = 0xFFFF;
    if (pool_full) {
        pool_full = 0;
    }
}

static struct binary_tree_node *pmm_node_alloc() {
    if (pool_full) {
        return _kalloc(sizeof(struct binary_tree_node));
    }

    for (uint64_t i = 0; i < BINARY_TREE_NODE_STATIC_POOL_SIZE; i++) {
        if (tree_node_static_pool[i].flags & BINARY_TREE_NODE_FREE) {
            tree_node_static_pool[i].flags &= ~BINARY_TREE_NODE_FREE;
            tree_node_static_pool[i].index = i;
            return &tree_node_static_pool[i];
        }
    }
    pool_full = 1;
    return _kalloc(sizeof(struct binary_tree_node));
}

/*
 * Free a node, just invoke _kfree( it not part of the static pool,
 * other wise clear it and return it to the pool
 */
static void pmm_node_free(struct binary_tree_node *node) {
    if (!(node->flags & BINARY_TREE_NODE_STATIC_POOL)) {
        _kfree(node);
        return;
    }

    if (node->parent != NULL) {
        if (node->parent->right == node) {
            node->parent->right = NULL;
        } else {
            node->parent->left = NULL;
        }
    }

    node->flags |= BINARY_TREE_NODE_FREE;
    node->parent = NULL;
    node->left = NULL;
    node->right = NULL;
    node->key = 0xFFFF;
    if (pool_full) {
        pool_full = 0;
    }
}

/*
 * Initialize a binary tree of type reg tree or rb, calls the appropriate function
 */
uint64_t init_tree(struct binary_tree *tree, uint64_t mode, uint64_t flags) {
    if (static_pool_init == false) {
        for (uint64_t i = 0; i < BINARY_TREE_NODE_STATIC_POOL_SIZE; i++) {
            tree_node_static_pool[i].flags = BINARY_TREE_NODE_STATIC_POOL | BINARY_TREE_NODE_FREE;
        }
        static_pool_init = true;
    }
    switch (mode) {
        case REGULAR_TREE:
            init_binary_tree(tree);
            return KERN_SUCCESS;

        case RED_BLACK_TREE:
            init_red_black_tree(tree);
            return KERN_SUCCESS;

        default:
            return BAD_TREE_MODE;
    }
}

/*
 *  Inserts a tree node, calls a different function depending on the type of tree
 */
uint64_t insert_tree_node(struct binary_tree *tree, void *data, uint64_t key) {
    switch (tree->mode) {
        case REGULAR_TREE:
            return insert_binary_tree(tree, data, key);


        case RED_BLACK_TREE:
            return insert_red_black_tree(tree, data, key);
        default:
            return BAD_TREE_MODE;
    }
}

/*
 * Removes a tree node, calls the appropriate function depending on tree type
 */
uint64_t remove_tree_node(struct binary_tree *tree, uint64_t key, void *address,
                          struct binary_tree_node *node /* Optional */) {
    switch (tree->mode) {
        case REGULAR_TREE:
            return remove_binary_tree(tree, key, address, node);
        case RED_BLACK_TREE:
            return remove_red_black_tree(tree, key);
        default:
            return BAD_TREE_MODE;
    }
}

/*
 * Frees a tree, if it is not free, frees all the nodes inside.
 * This function is untested.
 */
uint64_t destroy_tree(struct binary_tree *tree) {
    struct binary_tree_node *current = tree->root;
    struct binary_tree_node *parent = tree->root;

    if (current == NULL) {
        kfree(tree);
        return KERN_SUCCESS;
    }
    while (tree->node_count != 0) {
        /* Leaf node ? */
        if (current->left == NULL && current->right == NULL) {
            parent = current->parent;

            if (current->parent->left == current) {
                current->parent->left = NULL;
            } else {
                current->parent->right = NULL;
            }

            NODE_FREE(current)
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
    return KERN_SUCCESS;
}

/*
 * Inserts into a binary tree, does the typical binary search
 *  Current bigger? go right
 *  Current smaller? go left
 *  Continue until spot is found and insert beneath or in since we putting buckets in our tree
 */
uint64_t insert_binary_tree(struct binary_tree *tree, void *data, uint64_t key) {
    struct binary_tree_node *current = tree->root;

    if (current == NULL) {
        NODE_ALLOC(new_node)
        singly_linked_list_init(&new_node->data, 0);
        singly_linked_list_insert_head(&new_node->data, data);
        new_node->key = key;
        tree->root = new_node;
        tree->root->parent = NULL;
        tree->node_count++;
        return KERN_SUCCESS;
    }

    while (1) {
        // Sanity checks above and below mean that there shouldn't be any null nodes showing up here
        if (key == current->key) {
            singly_linked_list_insert_tail(&current->data, data);
            return KERN_SUCCESS;
        }

        if (key < current->key) {
            if (current->left == NULL) {
                NODE_ALLOC(new_node)
                singly_linked_list_init(&new_node->data, 0);
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
                return KERN_SUCCESS;
            }
            current = current->left;
        } else {
            if (current->right == NULL) {
                NODE_ALLOC(new_node)
                singly_linked_list_init(&new_node->data, 0);
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

                return KERN_SUCCESS;
            }
            current = current->right;
        }
    }
}

/*
 *  Looks up a key in the tree, does typical binary search.
 *  If the remove flag is set, it will remove the first entry
 *  in the bucket if the key is found, otherwise just will return the pointer.
 */
void *lookup_tree(struct binary_tree *tree, uint64_t key, uint8_t remove /* Flag to remove it from the tree*/) {
    struct binary_tree_node *current = tree->root;
    if (current == NULL) {
        return NULL; /* Indication of empty tree */
    }

    while (1) {
        // Sanity checks below mean that there shouldn't be any null nodes showing up here
        if (key == current->key) {
            void *return_value = current->data.head->data;;
            if (remove) {
                remove_tree_node(tree, key, current->data.head->data, current);
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
        } else {
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

uint64_t insert_red_black_tree(struct binary_tree *tree, void *data, uint64_t key) {
}

/*
 * Removes from a binary tree, node is optional this is the node to remove. If it is not null it will just remove right from the node,
 * otherwise it must traverse.
 *
 * Check for root node key, since address is passed because we have buckets, removal is done via singly_linked_list_remove_by_address otherwise
 * we would just grab the first entry in the list which may not be the correct entry.
 *
 * Do a standard tree traversal.
 *
 * when the node is found, there are a few possible actions.
 *
 * No children? Just remove.
 *
 * One child? Move that child up to the nodes place
 *
 * Two children? Bring the right sub-tree min value up (ie all lefts until a final right)
 *
 */
uint64_t remove_binary_tree(struct binary_tree *tree, uint64_t key, void *address,
                            struct binary_tree_node *node /* Optional */
                            /* This is required because there may be many of the same value so address needed to be passed as well */) {
    acquire_spinlock(&tree->lock);

    struct binary_tree_node *current = tree->root;
    struct binary_tree_node *parent = tree->root;

    if (node != NULL) {
        current = node;
        parent = node->parent;
    }

    if (current == NULL) {
        release_spinlock(&tree->lock);
        return VALUE_NOT_FOUND;
    }

    if (key == tree->root->key) {
        uint64_t ret = singly_linked_list_remove_node_by_address(&current->data, address);
        if (ret == NODE_NOT_FOUND) {
            serial_printf("ADDRESS %x.64\n", address);
            panic("Here");
        }
        release_spinlock(&tree->lock);
        return KERN_SUCCESS;
    }
    uint8_t depth = 0; /* Not sure if I am going to use this yet */
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
                if (singly_linked_list_remove_node_by_address(&current->data, address) == KERN_SUCCESS) {
                    release_spinlock(&tree->lock);
                    return KERN_SUCCESS;
                }
                release_spinlock(&tree->lock);
                return VALUE_NOT_FOUND;
            }


            if (current->left == NULL && current->right == NULL) {
                if (current->parent->left == current) {
                    current->parent->left = NULL;
                } else {
                    current->parent->right = NULL;
                }
                NODE_FREE(current);
                tree->node_count--;
                release_spinlock(&tree->lock);
                return KERN_SUCCESS;
            }


            /*
             * Handle right child is non-null while left child is
             */
            if (current->left == NULL) {
                if (current->parent->left == current) {
                    current->parent->left = current->right;
                } else {
                    current->parent->right = current->right;
                }

                if (current->right) {
                    current->right->parent = current->parent;
                }

                NODE_FREE(current);
                tree->node_count--;
                release_spinlock(&tree->lock);
                return KERN_SUCCESS;
            }
            /*
             *  Handle left child is non-null while right child is
             */

            if (current->right == NULL) {

                if (current->parent->left == current) {
                    current->parent->left = current->left;
                } else {
                    current->parent->right = current->left;
                }

                if (current->left) {
                    current->left->parent = current->parent;
                }
                NODE_FREE(current);
                tree->node_count--;
                release_spinlock(&tree->lock);
                return KERN_SUCCESS;
            }
            /*
             *
             *  Handle neither child is non-null
             *  Bring the right sub-tree min value up (ie all lefts until a final right)
             */
            parent = current->parent;
            struct binary_tree_node *right_min = current->right;
            struct binary_tree_node *right_min_parent = current; // Track the parent of right_min

            // Find the leftmost node in the right subtree
            while (right_min->left != NULL) {
                right_min_parent = right_min;
                right_min = right_min->left;
            }

            // Detach right_min from its original parent
            if (right_min_parent != current) {
                right_min_parent->left = right_min->right; // Reassign right_min's right child to its parent
            } else {
                right_min_parent->right = right_min->right; // Update the direct right child of `current`
            }

            // Reattach right_min to the current node's location
            right_min->left = current->left;
            right_min->right = current->right;

            // Update right_min's parent
            right_min->parent = parent;

            // Update the parent's reference to the new subtree
            if (parent != NULL) {
                if (parent->left == current) {
                    parent->left = right_min;
                } else {
                    parent->right = right_min;
                }
            }

            NODE_FREE(current);
            tree->node_count--;
            release_spinlock(&tree->lock);
            return KERN_SUCCESS;
        }

        if (key < current->key) {
            if (current->left == NULL) {
                release_spinlock(&tree->lock);
                return VALUE_NOT_FOUND;
            }

            current = current->left;
        } else {
            if (current->right == NULL) {
                release_spinlock(&tree->lock);
                return VALUE_NOT_FOUND;
            }
            current = current->right;
        }
    }
}


uint64_t remove_red_black_tree(struct binary_tree *tree, uint64_t key) {
}

/*
 * Init a red black tree
 */
void init_red_black_tree(struct binary_tree *tree) {
    tree->mode = RED_BLACK_TREE;
    tree->root->color = BLACK_NODE;
    tree->node_count = 0;
    initlock(&tree->lock, BTREE_LOCK);
}

/*
 * Init a reg bin tree
 */
void init_binary_tree(struct binary_tree *tree) {
    tree->mode = REGULAR_TREE;
    tree->node_count = 0;
    tree->root = NULL;
    initlock(&tree->lock, BTREE_LOCK);
}

