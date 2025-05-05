//
// Created by dustyn on 12/22/24.
//

#ifndef KERNEL_TMPFS_H
#define KERNEL_TMPFS_H

#include <include/data_structures/binary_tree.h>

#include "include/filesystem/vfs.h"
#include "include/data_structures/doubly_linked_list.h"

#define TMPFS_MAGIC 0x534F455077778034UL
#define PAGES_PER_TMPFS_ENTRY 1024UL
#define DIRECTORY_ENTRY_ARRAY_SIZE VNODE_MAX_DIRECTORY_ENTRIES * sizeof(uintptr_t)
#define MAX_TMPFS_ENTRIES VNODE_MAX_DIRECTORY_ENTRIES

struct tmpfs_directory_entries {
    struct tmpfs_node **entries;
};

struct tmpfs_superblock {
    struct tmpfs_filesystem_context *filesystem;
    struct binary_tree node_tree;
    uint64_t magic;
    uint64_t max_size;
    uint64_t block_size;
    uint64_t page_count;
    uint64_t tmpfs_node_count;
};

/*
 *  Doing it this way to require less node hopping to find a page offset. Can do O(1) lookups within
 *  PAGES_PER_TMPFS_ENTRY page lookups
 */
struct tmpfs_page_list_entry {
    uintptr_t **page_list;
    uint64_t number_of_pages;
};

struct sym_link_path {
    char *path;
};

struct tmpfs_node {
    struct tmpfs_superblock *superblock;
    struct tmpfs_node *parent_tmpfs_node;
    char node_name[VFS_MAX_NAME_LENGTH];
    uint64_t tmpfs_node_number;
    uint64_t tmpfs_node_size; // bytes if reg file, num dirents if directory
    uint64_t tmpfs_node_pages;
    uint8_t node_type;
    uint64_t t_flags;

    union {
        struct doubly_linked_list *page_list; //holds tmpfs page_list_entries as defined above
        struct tmpfs_directory_entries directory_entries;
        struct sym_link_path sym_link_path;
    };
};

struct tmpfs_filesystem_context {
    struct tmpfs_superblock *superblock;
    struct spinlock fs_lock;
    struct device *virtual_device;
};

struct vnode *tmpfs_lookup(struct vnode *vnode, char *name);

struct vnode *tmpfs_create(struct vnode *parent, char *name, uint8_t type);

void tmpfs_rename(const struct vnode *vnode, char *name);

int64_t tmpfs_write(struct vnode *vnode, uint64_t offset, const char *buffer, uint64_t bytes);

int64_t tmpfs_read(struct vnode *vnode, uint64_t offset, char *buffer, uint64_t bytes);

struct vnode *tmpfs_link(struct vnode *vnode, struct vnode *new_vnode, uint8_t type);

void tmpfs_unlink(struct vnode *vnode);

int64_t tmpfs_open(struct vnode *vnode);

void tmpfs_close(struct vnode *vnode, uint64_t handle);

void tmpfs_remove(const struct vnode *vnode);

void tmpfs_mkfs(uint64_t filesystem_id, char *directory_to_mount_onto);



#endif //KERNEL_TMPFS_H
