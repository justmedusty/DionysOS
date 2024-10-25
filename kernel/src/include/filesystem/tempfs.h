//
// Created by dustyn on 9/18/24.
//

#pragma once
#include "include/types.h"
#include "include/filesystem/vfs.h"

#define TEMPFS_BLOCKSIZE 2048
#define TEMPFS_MAGIC 0x7777777777777777
#define TEMPFS_VERSION 1
#define MAX_LEVEL_INDIRECTIONS 5
#define MAX_FILENAME_LENGTH 128 /* This number is here so we can fit 2 inodes in 1 2048 block */
//marks this block as an indirection block , an array of 64bit block pointers
#define INDRECTION_HEADER 0x123456789ABCEFEC


extern struct vnode_operations tempfs_vnode_ops;

struct tempfs_superblock {
  uint64 magic;
  uint64 version;
  uint64 blocksize;
  uint64 blocks;
  uint64 inodes;
  uint64 size;
  /* Both bitmap entries hold block pointers */
  uint64 inode_bitmap[3]; /* Can hold 50k inodes asuming 2048 size */
  uint64 block_bitmap[12]; /* Can hold 196k blocks assuming the 2048 size doesn't change */
};

//2 inodes per block
struct tempfs_inode {
  uint32 uid;
  uint32 gid;
  char name[MAX_FILENAME_LENGTH];
  uint64 size;
  uint64 blocks[12]; /* Will point to logical block numbers */
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

