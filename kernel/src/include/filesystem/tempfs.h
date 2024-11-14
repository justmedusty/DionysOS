//
// Created by dustyn on 9/18/24.
//

#pragma once
#include "include/types.h"
#include "include/filesystem/vfs.h"
#define INITRAMFS 0 /* Just for ramdisk 0 id purposes*/

#define TEMPFS_BLOCKSIZE 1024
#define TEMPFS_MAGIC 0x7777777777777777
#define TEMPFS_VERSION 1
#define MAX_LEVEL_INDIRECTIONS 3
#define MAX_FILENAME_LENGTH 128 /* This number is here so we can fit 2 inodes in 1 2048 block */

#define DEFAULT_TEMPFS_SIZE (19705 * TEMPFS_BLOCKSIZE)
#define MAX_BLOCKS_IN_INODE (((((TEMPFS_NUM_BLOCK_POINTERS_PER_INODE) * TEMPFS_BLOCKSIZE) * NUM_BLOCKS_IN_INDIRECTION_BLOCK) * NUM_BLOCKS_IN_INDIRECTION_BLOCK) * NUM_BLOCKS_IN_INDIRECTION_BLOCK)

#define NUM_BLOCKS_IN_INDIRECTION_BLOCK ((TEMPFS_BLOCKSIZE / sizeof(uint64)))

#define TEMPFS_DIRECTORY 0
#define TEMPFS_REG_FILE 1
#define TEMPFS_SYMLINK 2

#define TEMPFS_NUM_INODE_POINTER_BLOCKS 6
#define TEMPFS_NUM_BLOCK_POINTER_BLOCKS 114

#define NUM_INODES_PER_BLOCK ((TEMPFS_BLOCKSIZE / sizeof(struct tempfs_inode)))
#define TEMPFS_INODES_PER_BLOCK (TEMPFS_BLOCKSIZE / TEMPFS_INODE_SIZE)
#define TEMPFS_BLOCK_POINTER_SIZE sizeof(uint64)

#define BITMAP_TYPE_BLOCK 0
#define BITMAP_TYPE_INODE 1

#define BITMAP_ACTION_SET 1
#define BITMAP_ACTION_CLEAR 0

#define NUM_BLOCKS_DIRECT 10
#define NUM_BLOCKS_SINGULAR_INDIRECTION (NUM_BLOCKS_IN_INDIRECTION_BLOCK)
#define NUM_BLOCKS_DOUBLE_INDIRECTION (NUM_BLOCKS_IN_INDIRECTION_BLOCK * NUM_BLOCKS_IN_INDIRECTION_BLOCK)
#define NUM_BLOCKS_TRIPLE_INDIRECTION (NUM_BLOCKS_IN_INDIRECTION_BLOCK * NUM_BLOCKS_IN_INDIRECTION_BLOCK * NUM_BLOCKS_IN_INDIRECTION_BLOCK)

#define TEMPFS_NUM_BLOCK_POINTERS_PER_INODE 10
#define TEMPFS_INODE_SIZE sizeof(struct tempfs_inode)
#define TEMPFS_NUM_INODES TEMPFS_NUM_INODE_POINTER_BLOCKS * TEMPFS_BLOCKSIZE * 8
#define TEMPFS_NUM_BLOCKS TEMPFS_NUM_BLOCK_POINTER_BLOCKS * TEMPFS_BLOCKSIZE * 8

#define TEMPFS_BLOCK_UNALLOCATED 0xFFFFFFFFFFFFFFFF
#define TEMPFS_BUFFER_TOO_SMALL 0x1
#define TEMPFS_NOT_A_DIRECTORY 0x2
#define TEMPFS_CANT_ALLOCATE_BLOCKS_FOR_DIR 0x3

#define TEMPFS_ERROR 0x6
#define SUCCESS 0

#define TEMPFS_MAX_FILES_IN_DIRECTORY ((TEMPFS_NUM_BLOCK_POINTERS_PER_INODE * TEMPFS_BLOCKSIZE) / sizeof(struct tempfs_directory_entry))
/*
 *  These macros make it easier to change the size created by tempfs_init by just modifying values of
 *  TEMPFS_NUM_INODE_POINTER_BLOCKS and TEMPFS_NUM_BLOCK_POINTER_BLOCKS
 */
#define TEMPFS_SUPERBLOCK 0
#define TEMPFS_START_INODE_BITMAP 1
#define TEMPFS_START_BLOCK_BITMAP (TEMPFS_START_INODE_BITMAP + TEMPFS_NUM_INODE_POINTER_BLOCKS)
#define TEMPFS_START_INODES (TEMPFS_START_BLOCK_BITMAP + TEMPFS_NUM_BLOCK_POINTER_BLOCKS)
#define TEMPFS_START_BLOCKS (TEMPFS_START_INODES + (TEMPFS_NUM_INODE_POINTER_BLOCKS * TEMPFS_BLOCKSIZE * 8))

#define TEMPFS_GET_BLOCK_NUMBER_OF_INODE(inode_number)
extern struct vnode_operations tempfs_vnode_ops;

struct tempfs_superblock {
    uint64 magic;
    uint64 version;
    uint64 block_size;
    uint64 num_blocks;
    uint64 num_inodes;
    uint64 total_size;
    /* Both bitmap entries hold n block pointers */
    uint64 inode_bitmap_pointers[TEMPFS_NUM_INODE_POINTER_BLOCKS]; /* Can hold 50k inodes assuming 1024 size */
    uint64 block_bitmap_pointers[TEMPFS_NUM_BLOCK_POINTER_BLOCKS];
    uint64 inode_start_pointer; /* Where inodes start */
    uint64 block_start_pointer; /* Where blocks start */
};

_Static_assert(sizeof(struct tempfs_superblock) == TEMPFS_BLOCKSIZE, "Tempfs Superblock not the proper size");

//4 inodes per block
struct tempfs_inode {
    uint16 uid;
    uint16 inode_number;
    uint16 type;
    uint16 refcount;
    char name[MAX_FILENAME_LENGTH];
    uint64 size;
    uint64 blocks[10]; /* Will point to logical block numbers */
    uint64 single_indirect;
    uint64 double_indirect;
    uint64 triple_indirect;
    uint64 reserved;
};

/*
 * As a note, we will NOT allow indirect blocks with directories for this filesystem,
 * because 50-70 dirents in one inode is more than enough for us. We want to kill things
 * simple after all.
 *
 * As it stands right now, 7 tempfs_dir entries will fit into a block, * 13 means
 * that a directory can hold 91 entries. More than enough.
 */
struct tempfs_directory_entry {
    char name[MAX_FILENAME_LENGTH];
    uint32 inode_number;
    uint16 type;
    uint16 device_number;
    uint32 refcount;
    uint32 size;
};


struct tempfs_byte_offset_indices {
    uint16 top_level_block_number;
    uint16 third_level_block_number;
    uint16 second_level_block_number;
    uint16 first_level_block_number;
    uint16 block_number;
    uint16 levels_indirection;
};

_Static_assert(sizeof(struct tempfs_inode) % 256 == 0, "Tempfs inode not the proper size");


void tempfs_init();
void tempfs_mkfs(uint64 ramdisk_id);
uint64 tempfs_read(struct vnode* vnode, uint64 offset, char* buffer, uint64 bytes);
uint64 tempfs_write(struct vnode* vnode, uint64 offset, char* buffer, uint64 bytes);
uint64 tempfs_stat(struct vnode* vnode, uint64 offset, char* buffer, uint64 bytes);
struct vnode* tempfs_lookup(struct vnode* vnode, char* name);
struct vnode* tempfs_create(struct vnode* vnode, struct vnode* new_vnode, uint8 vnode_type);
void tempfs_close(struct vnode* vnode);
struct vnode* tempfs_link(struct vnode* vnode, struct vnode* new_vnode);
void tempfs_unlink(struct vnode* vnode);
void tempfs_remove(struct vnode* vnode);
void tempfs_rename(struct vnode* vnode, char* new_name);
uint64 tempfs_open(struct vnode* vnode);

