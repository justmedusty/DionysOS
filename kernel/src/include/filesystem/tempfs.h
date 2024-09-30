//
// Created by dustyn on 9/18/24.
//

#pragma once
#include "include/types.h"
#include "include/filesystem/vfs.h"

#define TEMPFS_BLOCKSIZE 512
#define TEMPFS_MAGIC 0x7777777777777777
#define TEMPFS_VERSION 1

extern struct vnode_operations tempfs_vnode_ops;

struct tempfs_superblock {
  uint64 magic;
  uint64 version;
  uint64 blocksize;
  uint64 blocks;
  uint64 inodes;
  uint64 size;
  uint64 inode_bitmap;
  uint64 block_bitmap;
};

struct tempfs_inode {
  uint32 uid;
  uint32 gid;
  uint64 mode;
  uint64 size;
  uint64 blocks[];
};

struct tempfs_block {
  uint64 data[sizeof(uint64) * (TEMPFS_BLOCKSIZE / sizeof(uint64))];
};

void tempfs_init();
uint64 tempfs_read(struct vnode *vnode,uint64 offset, char *buffer, uint64 bytes);
uint64 tempfs_write(struct vnode *vnode,uint64 offset, char *buffer, uint64 bytes);
uint64 tempfs_stat(struct vnode *vnode,uint64 offset, char *buffer, uint64 bytes);
struct vnode *tempfs_lookup(struct vnode *vnode, char *name);
struct vnode *tempfs_create(struct vnode *vnode,struct vnode *new_vnode,uint8 vnode_type);
void tempfs_close(struct vnode *vnode);
struct vnode *tempfs_link(struct vnode *vnode,struct vnode *new_vnode);
void tempfs_unlink(struct vnode *vnode);
void tempfs_remove(struct vnode *vnode);
void tempfs_rename(struct vnode *vnode,char *new_name);
uint64 tempfs_open(struct vnode *vnode);

