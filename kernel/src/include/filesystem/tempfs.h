//
// Created by dustyn on 9/18/24.
//

#pragma once
#include "include/types.h"


#define TEMPFS_BLOCKSIZE 512
#define TEMPFS_MAGIC 0x77737773
#define TEMPFS_VERSION 1

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



