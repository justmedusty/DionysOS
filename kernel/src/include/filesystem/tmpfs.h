//
// Created by dustyn on 12/22/24.
//

#ifndef KERNEL_TMPFS_H
#define KERNEL_TMPFS_H
#include "include/filesystem/vfs.h"
#include "include/data_structures/doubly_linked_list.h"

#define PAGES_PER_TMPFS_ENTRY 1024

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
    uint8_t node_type;
    struct doubly_linked_list page_list; //holds tmpfs page_list_entries
    uint64_t t_flags;
};

struct tmpfs_directory_entry {
    char name[VFS_MAX_NAME_LENGTH];
    uint8_t node_type;
    uint64_t t_node_number;
    uint64_t t_node_size;
};

/*
 *  Doing it this way to require less node hopping to find a page offset. Can do O(1) lookups within
 *  PAGES_PER_TMPFS_ENTRY page lookups
 */
struct tmpfs_page_list_entry {
    char **page_list;
    uint64_t number_of_pages;
};

struct vnode *tmpfs_lookup(struct vnode *vnode, char *name);
struct vnode *tmpfs_create(struct vnode *parent, char *name,uint8_t type);
void tmpfs_rename(const struct vnode *vnode, char *name);
uint64_t tmpfs_write(struct vnode *vnode, uint64_t offset, char *buffer, uint64_t bytes);
uint64_t tmpfs_read(struct vnode *vnode, uint64_t offset, char *buffer, uint64_t bytes);
struct vnode *tmpfs_link(struct vnode *vnode, struct vnode *new_vnode, uint8_t type);
void tmpfs_unlink(struct vnode *vnode);
uint64_t tmpfs_open(struct vnode *vnode);
void tmpfs_close(struct vnode *vnode, uint64_t handle);
void tmpfs_remove(const struct vnode *vnode);
#endif //KERNEL_TMPFS_H
