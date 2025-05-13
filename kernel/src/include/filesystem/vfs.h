//
// Created by dustyn on 9/14/24.
//

#pragma once

#include "include/data_structures/spinlock.h"
#include <stddef.h>
#include "include/definitions/definitions.h"

#define VFS_MAX_NAME_LENGTH 128
#define VFS_STATIC_POOL_SIZE 100

#define ROOT_INODE 0
#define CURRENT_DIRECTORY_NAME "."
#define PARENT_DIRECTORY_NAME ".."
#define ROOT_DIRECTORY_NAME "/"

extern struct vnode vfs_root;



/* Device Types */
enum vnode_device_types {
    VNODE_DEV_TEMP = 0,
    VNODE_DEV_IDE = 1,
    VNODE_DEV_NVME = 2,
    VNODE_DEV_SATA = 3,
};

/* Node types */
enum vnode_types {
    VNODE_DIRECTORY = 0,
    VNODE_FILE = 1,
    VNODE_HARD_LINK = 2,
    VNODE_BLOCK_DEV = 3,
    VNODE_CHAR_DEV = 4,
    VNODE_NET_DEV = 5,
    VNODE_SPECIAL = 6,
    VNODE_SPECIAL_FILE = 7,
    VNODE_SYM_LINK = 8,
};
#define VALID_VNODE_CASES \
case VNODE_DIRECTORY: \
case VNODE_FILE: \
case VNODE_HARD_LINK: \
case VNODE_BLOCK_DEV: \
case VNODE_CHAR_DEV: \
case VNODE_NET_DEV: \
case VNODE_SPECIAL: \
case VNODE_SPECIAL_FILE: \
case VNODE_SYM_LINK:

/* FS Types */
#define VNODE_FS_VNODE_ROOT 0xE
#define VNODE_FS_TMPFS 0x0
#define VNODE_FS_DIOSFS 0xF
#define VNODE_FS_EXT2 0x10

#define NUM_HANDLES 32
/* Vnode flags */
#define VNODE_STATIC_POOL 2
#define VNODE_CHILD_MEMORY_ALLOCATED 1

/* Vnode limits */
#define VNODE_MAX_DIRECTORY_ENTRIES 64 // 4 pages

struct vnode {
    struct vnode *vnode_parent;
    struct vnode **vnode_children;
    uint8_t num_children;
    struct vnode_operations *vnode_ops;
    struct vnode *mounted_vnode;
    char vnode_name[VFS_MAX_NAME_LENGTH];
    void *filesystem_object;
    uint64_t vnode_size;
    uint64_t last_updated;
    uint64_t vnode_inode_number;
    uint64_t vnode_flags;
    uint16_t vnode_type;
    uint64_t vnode_filesystem_id; /* will be empty if this is a device or otherwise non-file/directory node */
    uint16_t vnode_refcount; // Pulled from actual inode or equivalent structure
    uint16_t vnode_active_references;
    //Will be used to hold how many processes have this either open or have it as their CWD so I can free when it gets to 0
    uint64_t vnode_device_id;
    uint16_t is_mount_point;
    uint16_t is_mounted;
    uint64_t is_cached;
};

struct date_time {
    uint16_t month;
    uint16_t year;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

struct vnode_stat {
    struct vnode *vnode;
    uint64_t vnode_size;
    uint64_t block_count;
};

struct virtual_handle {
    uint64_t handle_id;
    struct process *process;
    struct vnode *vnode;
    uint64_t offset; // for lseek or equivalent
};

struct virtual_handle_list {
    struct doubly_linked_list *handle_list;
    uint64_t num_handles;
    uint32_t handle_id_bitmap;
};

struct vnode_operations {
    struct vnode *(*lookup)(struct vnode *vnode, char *name);

    struct vnode *(*create)(struct vnode *parent, char *name, uint8_t vnode_type);

    void (*remove)(const struct vnode *vnode);

    void (*rename)(const struct vnode *vnode, char *new_name);

    int64_t (*write)(struct vnode *vnode, uint64_t offset, const char *buffer, uint64_t bytes);

    int64_t (*read)(struct vnode *vnode, uint64_t offset, char *buffer, uint64_t bytes);

    struct vnode *(*link)(struct vnode *vnode, struct vnode *new_vnode, uint8_t type);

    void (*unlink)(struct vnode *vnode);

    int64_t (*open)(struct vnode *vnode);

    void (*close)(struct vnode *vnode, uint64_t handle);
};

void vnode_directory_alloc_children(struct vnode *vnode);

struct vnode *vnode_create(char *path, char *name, uint8_t vnode_type);

struct vnode *find_vnode_child(struct vnode *vnode, char *token);

int64_t vnode_write(struct vnode *vnode, const uint64_t offset, const uint64_t bytes, const char *buffer);

int64_t vnode_read(struct vnode *vnode, const uint64_t offset, const uint64_t bytes, char *buffer);

int64_t vnode_unmount(struct vnode *vnode);

int64_t vnode_mount(struct vnode *mount_point, struct vnode *mounted_vnode);

struct vnode *find_vnode_child(struct vnode *vnode, char *token);

int32_t vnode_remove(struct vnode *vnode, char *path);

struct vnode *vnode_lookup(char *path);

int64_t vnode_unlink(struct vnode *link);

struct vnode *vnode_link(struct vnode *vnode, struct vnode *new_vnode, uint8_t type);

void vfs_init();

char *vnode_get_canonical_path(
    struct vnode *vnode); /* Not sure if this will need to be externally linked but I'll include it for now */
/* These two are only exposed because other filesystems may return vnodes up to the abstraction layer above them */
struct vnode *vnode_alloc();

void vnode_free(struct vnode *vnode);

int64_t vnode_open(char *path);

void vnode_close(uint64_t handle);

void vnode_rename(struct vnode *vnode, char *new_name);

struct vnode *handle_to_vnode(uint64_t handle_id);

struct vnode *vnode_create_without_path(struct vnode *parent_directory, char *name, uint8_t vnode_type);
