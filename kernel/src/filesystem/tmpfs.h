//
// Created by dustyn on 12/22/24.
//

#ifndef KERNEL_TMPFS_H
#define KERNEL_TMPFS_H
#include "include/filesystem/vfs.h"
#include "include/data_structures/doubly_linked_list.h"

enum tmpfs_types {
    DIRECTORY = 0,
    REG_FILE = 1,
    HARD_LINK = 2,
    BLOCK_DEV = 3,
    CHAR_DEV = 4,
    NET_DEV = 5,
    SPECIAL = 6,
    SPECIAL_FILE = 7,
    SYM_LINK = 8,
};

struct tmpfs_node {
    struct tmpfs_node *parent_t_node;
    char node_name[VFS_MAX_NAME_LENGTH];
    uint64_t t_node_number;
    uint64_t t_node_size;
    uint64_t t_node_pages;
    uint64_t node_type;
    struct doubly_linked_list page_list;
    uint64_t t_flags;
};



struct vnode_operations tmpfs_ops = {

};
#endif //KERNEL_TMPFS_H
