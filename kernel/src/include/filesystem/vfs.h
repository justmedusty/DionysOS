//
// Created by dustyn on 9/14/24.
//

#pragma once
#include "include/types.h"
#include "include/data_structures/spinlock.h"
#include "include/definitions.h"

#define VFS_MAX_NAME_LENGTH 128

#define VFS_STATIC_POOL_SIZE 100

extern struct vnode vfs_root;


/* Error responses */
#define SUCCESS 0
#define WRONG_TYPE 1
#define ALREADY_MOUNTED 2
#define ALREADY_OPENED 3
#define NO_ACCESS 4
#define NOT_MOUNTED 5

/* Device Types */
#define VNODE_DEV_TEMP 0
#define VNODE_DEV_IDE 1
#define VNODE_DEV_NVME 2
#define VNODE_DEV_SATA 3

/* Node types */
#define VNODE_DIRECTORY 0
#define VNODE_FILE 1
#define VNODE_LINK 2
#define VNODE_BLOCKDEV 3
#define VNODE_CHARDEV 4
#define VNODE_NETDEV 5
#define VNODE_SPECIAL 6
#define VNODE_SPECIAL_FILE 7

/* FS Types */
#define VNODE_FS_TEMPFS 0xF
#define VNODE_FS_EXT2 0x10

/* Vnode flags */
#define VNODE_SYMLINK 1 << 0
#define VNODE_HARDLINK 1 << 1
#define VNODE_STATIC_POOL 1<<63
#define VNODE_CHILD_MEMORY_ALLOCATED (1 << 62)

/* Vnode limits */
#define VNODE_MAX_DIRECTORY_ENTRIES 64 // 4 pages

struct vnode {
    struct vnode* vnode_parent;
    struct vnode **vnode_children;
    uint8 num_children;
    struct vnode_operations *vnode_ops;
    struct vnode* mounted_vnode;
    char vnode_name[VFS_MAX_NAME_LENGTH];
    void *filesystem_object;
    uint64 vnode_size;
    uint64 last_updated;
    uint64 vnode_inode_number;
    uint64 vnode_flags;
    uint16 vnode_type;
    uint64 vnode_filesystem_id; /* will be empty if this is a device or otherwise non-file/directory node */
    uint16 vnode_refcount; // Pulled from actual inode or equivalent structure
    uint16 vnode_active_references; //Will be used to hold how many processes have this either open or have it as their CWD so I can free when it gets to 0
    uint64 vnode_device_id;
    uint16 is_mount_point;
    uint64 is_cached;
};
struct date_time {
    uint16 month;
    uint16 year;
    uint8 day;
    uint8 hour;
    uint8 minute;
    uint8 second;
};
struct vnode_stat {
    struct vnode* vnode;
    uint64 vnode_size;

};

struct virtual_handle {
    uint64 handle_id;
    struct process *process;
    struct vnode *vnode;
    uint64 offset; // for lseek or equivalent

};


struct vnode_operations {
    struct vnode* (*lookup)(struct vnode* vnode, char* name);
    struct vnode* (*create)(struct vnode* vnode, struct vnode* new_vnode,uint8 vnode_type);
    void (*remove)(struct vnode* vnode);
    void (*rename)(struct vnode* vnode, char* new_name);
    uint64 (*write)(struct vnode* vnode,uint64 offset,char *buffer,uint64 bytes);
    uint64 (*read)(struct vnode* vnode,uint64 offset,char *buffer,uint64 bytes);
    struct vnode* (*link)(struct vnode* vnode, struct vnode* new_vnode);
    void (*unlink)(struct vnode* vnode);
    uint64 (*open)(struct vnode* vnode);
    void (*close)(struct vnode* vnode);
};

void vnode_directory_alloc_children(struct vnode* vnode);
struct vnode* find_vnode_child(struct vnode* vnode, char* token);
uint64 vnode_write(struct vnode* vnode, uint64 offset, uint64 bytes,char *buffer);
uint64 vnode_read(struct vnode* vnode, uint64 offset, uint64 bytes,char *buffer);
uint64 vnode_unmount(struct vnode* vnode);
uint64 vnode_mount(struct vnode* mount_point, struct vnode* mounted_vnode);
struct vnode* find_vnode_child(struct vnode* vnode, char* token);
void vnode_remove(struct vnode* vnode);
struct vnode* vnode_lookup(char* path);
void vfs_init();

/* These two are only exposed because other filesystems may return vnodes up to the abstraction layer above them */
struct vnode* vnode_alloc();
void vnode_free(struct vnode* vnode);