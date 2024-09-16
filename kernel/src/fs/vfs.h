//
// Created by dustyn on 9/14/24.
//

#pragma once
#include "include/types.h"

#define VFS_MAX_NAME 255

struct vnode {
  struct vnode *vnode_parent;
  struct vnode **vnode_children;
  struct vnode_operations *vnode_ops;
  char vnode_name[VFS_MAX_NAME];
  uint64 vnode_type;
  uint64 vnode_inode_number;
  uint64 vnode_xattrs;
  uint64 vnode_refcount;
  uint64 vnode_flags;
  uint64 vnode_device_id;

 };


struct vnode_operations {
   uint64 (*lookup)(struct vnode *vnode, char *name);
   uint64 (*create)(struct vnode *vnode, struct vnode **new_vnode);
   uint64 (*remove)(struct vnode *vnode);
   uint64 (*rename)(struct vnode *vnode, char *name);
   uint64 (*update)(struct vnode *vnode);
};