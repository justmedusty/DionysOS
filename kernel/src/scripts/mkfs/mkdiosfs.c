//
// Created by dustyn on 12/15/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mkdiosfs.h"

void diosfs_get_size_info(struct diosfs_size_calculation* size_calculation, size_t gigabytes,size_t block_size) {
  if (gigabytes == 0) {
    return;
  }

  uint64_t size_bytes = gigabytes << 30;

  size_calculation->total_blocks = size_bytes / block_size;

  uint64_t split = size_calculation->total_blocks / 32;

  size_calculation->total_inodes = split * (block_size / sizeof(struct diosfs_inode));
  size_calculation->total_data_blocks = size_calculation->total_blocks - split;
  size_calculation->total_block_bitmap_blocks = size_calculation->total_blocks / block_size / 8;
  size_calculation->total_inode_bitmap_blocks = size_calculation->total_inodes / 8 / block_size;

  size_calculation->total_blocks += 1; // superblock;
}

// mkdiosfs
int main(int argc, char **argv){

  if(argc != 3){
    printf("Usage: mkdiosfs name size\n");
  }

  }