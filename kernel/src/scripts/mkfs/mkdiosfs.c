//
// Created by dustyn on 12/15/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mkdiosfs.h"

uint64_t strtoll_wrapper(char* arg);

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
int main(const int argc, char **argv){

  if(argc < 4){
    printf("Usage: mkdiosfs name.img size-gigs\n");
    printf("You can add an archive via: mkdiosfs name.img size-gigs block-size --archive archive-name\n");
    exit(1);
  }

  uint64_t arg2 = strtoll_wrapper(argv[2]);

  if (arg2 > 8) {
    printf("This tool does not support images above 8GB, pick a size between 1 and 8 GB\n");
    exit(1);
  }

  FILE *f = fopen(argv[1],"wb");

  if(!f) {
    printf("Error creating file\n");
    exit(1);
  }

  char *disk_buffer = malloc(arg2);

  if(!disk_buffer) {
    printf("Error allocating disk buffer\n");
    perror("malloc");
    exit(1);
  }

  char* block = malloc(DIOSFS_BLOCKSIZE);

  if(!block) {
    printf("Error allocating block\n");
    perror("malloc");
    exit(1);
  }
  memset(block,0,*argv[3]);
  struct diosfs_superblock *superblock = (struct diosfs_superblock *) block;

  struct diosfs_size_calculation size_calculation = {0};

  diosfs_get_size_info(&size_calculation,arg2,DIOSFS_BLOCKSIZE);

  superblock->magic = DIOSFS_MAGIC;
  superblock->version = DIOSFS_VERSION;
  superblock->block_size = DIOSFS_BLOCKSIZE;
  superblock->num_blocks = size_calculation.total_blocks;
  superblock->num_inodes = size_calculation.total_inodes;
  superblock->inode_start_pointer = DIOSFS_START_INODES;
  superblock->block_start_pointer = DIOSFS_START_BLOCKS;
  superblock->inode_bitmap_pointers_start = 1;
  superblock->block_bitmap_pointers_start = DIOSFS_START_BLOCK_BITMAP;
  superblock->block_bitmap_size = size_calculation.total_block_bitmap_blocks;
  superblock->inode_bitmap_size = size_calculation.total_inode_bitmap_blocks;
  superblock->total_size = size_calculation.total_blocks * superblock->block_size;



  free(block);
  free(disk_buffer);

}

uint64_t strtoll_wrapper(char* arg) {
  char *end_ptr;
  uint64_t result = strtoull(arg,&end_ptr,10);
  if (*end_ptr != '\0') {
    printf("Error converting string\n");
    exit(1);
  }

  return result;
}