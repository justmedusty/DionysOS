//
// Created by dustyn on 9/14/24.
//

#pragma once
#include "include/types.h"
#include "include/data_structures/spinlock.h"
#include "include/definitions.h"

#define VFS_MAX_PATH 255

struct vnode *vfs_root;

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

struct spinlock vfs_lock;

struct vnode {
    struct vnode* vnode_parent;
    struct vnode** vnode_children;
    struct vnode_operations* vnode_ops;
    struct vnode* mounted_vnode;
    char vnode_name[VFS_MAX_PATH];
    uint64 vnode_inode_number;
    uint32 vnode_xattrs;
    uint32 vnode_flags;
    uint8 vnode_type;
    uint8 vnode_refcount;
    uint8 vnode_device_id;
    uint8  is_mount;
    uint8  is_cached;
};


struct vnode_operations {
    struct vnode* (*lookup)(struct vnode* vnode, char* name);
    struct vnode* (*create)(struct vnode* vnode, struct vnode** new_vnode);
    void (*remove)(struct vnode* vnode);
    void (*rename)(struct vnode* vnode, char* new_name);
    struct vnode* (*update)(struct vnode* vnode);
    void (*mount)(struct vnode* mount_point, struct vnode* target);
    void (*unmount)(struct vnode* mount_point);
    struct vnode* (*link)(struct vnode* vnode, struct vnode* new_vnode);
    void (*unlink)(struct vnode* vnode);
};
