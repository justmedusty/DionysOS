//
// Created by dustyn on 9/18/24.
//

#pragma once
#include "include/types.h"
#include "stdint.h"
#include "include/filesystem/vfs.h"
#define INITRAMFS 0 /* Just for ramdisk 0 id purposes*/

#define TEMPFS_BLOCKSIZE 1024
#define TEMPFS_MAGIC 0x7777777777777777
#define TEMPFS_VERSION 1
#define MAX_FILENAME_LENGTH 128 /* This number is here so we can fit 2 inodes in 1 2048 block */

#define DEFAULT_TEMPFS_SIZE (TEMPFS_BLOCKSIZE /* super block */ + (TEMPFS_NUM_INODE_POINTER_BLOCKS * TEMPFS_BLOCKSIZE) + (TEMPFS_NUM_BLOCK_POINTER_BLOCKS  * TEMPFS_BLOCKSIZE) + (TEMPFS_NUM_BLOCKS  * TEMPFS_BLOCKSIZE) + (TEMPFS_NUM_INODES  * TEMPFS_BLOCKSIZE))
#define MAX_BLOCKS_IN_INODE (((((NUM_BLOCKS_DIRECT) * TEMPFS_BLOCKSIZE) * NUM_BLOCKS_IN_INDIRECTION_BLOCK) * NUM_BLOCKS_IN_INDIRECTION_BLOCK) * NUM_BLOCKS_IN_INDIRECTION_BLOCK)

#define NUM_BLOCKS_IN_INDIRECTION_BLOCK ((TEMPFS_BLOCKSIZE / sizeof(uint64_t)))

#define TEMPFS_DIRECTORY 0
#define TEMPFS_REG_FILE 1
#define TEMPFS_SYMLINK 2

#define TEMPFS_NUM_INODE_POINTER_BLOCKS 16
#define TEMPFS_NUM_BLOCK_POINTER_BLOCKS 128

#define TEMPFS_NUM_BLOCKS (TEMPFS_NUM_BLOCK_POINTER_BLOCKS * TEMPFS_BLOCKSIZE * 8)
#define TEMPFS_NUM_INODES (TEMPFS_NUM_INODE_POINTER_BLOCKS * TEMPFS_BLOCKSIZE * 8)

#define NUM_INODES_PER_BLOCK ((TEMPFS_BLOCKSIZE / sizeof(struct tempfs_inode)))
#define TEMPFS_INODES_PER_BLOCK (TEMPFS_BLOCKSIZE / TEMPFS_INODE_SIZE)
#define TEMPFS_BLOCK_POINTER_SIZE sizeof(uint64_t)

#define BITMAP_TYPE_BLOCK 0
#define BITMAP_TYPE_INODE 1

#define BITMAP_ACTION_SET 1
#define BITMAP_ACTION_CLEAR 0

#define NUM_BLOCKS_DIRECT 10
#define NUM_BLOCKS_SINGULAR_INDIRECTION (NUM_BLOCKS_IN_INDIRECTION_BLOCK)
#define NUM_BLOCKS_DOUBLE_INDIRECTION (NUM_BLOCKS_IN_INDIRECTION_BLOCK * NUM_BLOCKS_IN_INDIRECTION_BLOCK)
#define NUM_BLOCKS_TRIPLE_INDIRECTION (NUM_BLOCKS_IN_INDIRECTION_BLOCK * NUM_BLOCKS_IN_INDIRECTION_BLOCK * NUM_BLOCKS_IN_INDIRECTION_BLOCK)

#define TEMPFS_INODE_SIZE sizeof(struct tempfs_inode)
#define TEMPFS_NUM_INODES TEMPFS_NUM_INODE_POINTER_BLOCKS * TEMPFS_BLOCKSIZE * 8
#define TEMPFS_NUM_BLOCKS TEMPFS_NUM_BLOCK_POINTER_BLOCKS * TEMPFS_BLOCKSIZE * 8

#define TEMPFS_BLOCK_UNALLOCATED 0xFFFFFFFFFFFFFFFF
#define TEMPFS_BUFFER_TOO_SMALL 0x1
#define TEMPFS_NOT_A_DIRECTORY 0x2
#define TEMPFS_CANT_ALLOCATE_BLOCKS_FOR_DIR 0x3

#define TEMPFS_ERROR 0x6
#define SUCCESS 0
#define NOT_FOUND 1

#define TEMPFS_MAX_FILES_IN_DIRENT_BLOCK ((fs->superblock->block_size / sizeof(struct tempfs_directory_entry)))
#define TEMPFS_MAX_FILES_IN_DIRECTORY ((NUM_BLOCKS_DIRECT * fs->superblock->block_size) / sizeof(struct tempfs_directory_entry))
/*
 *  These macros make it easier to change the size created by tempfs_init by just modifying values of
 *  TEMPFS_NUM_INODE_POINTER_BLOCKS and TEMPFS_NUM_BLOCK_POINTER_BLOCKS
 */
#define TEMPFS_SUPERBLOCK 0
#define TEMPFS_START_INODE_BITMAP 1
#define TEMPFS_START_BLOCK_BITMAP (TEMPFS_START_INODE_BITMAP + TEMPFS_NUM_INODE_POINTER_BLOCKS)
#define TEMPFS_START_INODES (TEMPFS_START_BLOCK_BITMAP + TEMPFS_NUM_BLOCK_POINTER_BLOCKS)
#define TEMPFS_START_BLOCKS (TEMPFS_START_INODES + (TEMPFS_NUM_INODE_POINTER_BLOCKS * TEMPFS_BLOCKSIZE * 8) / NUM_INODES_PER_BLOCK)

#define TEMPFS_GET_BLOCK_NUMBER_OF_INODE(inode_number)
extern struct vnode_operations tempfs_vnode_ops;

struct tempfs_superblock {
    uint64_t magic;
    uint64_t version;
    uint64_t block_size;
    uint64_t num_blocks;
    uint64_t num_inodes;
    uint64_t total_size;
    /* Both bitmap entries hold n block pointers */
    uint64_t inode_bitmap_size;
    uint64_t block_bitmap_size;
    uint64_t inode_bitmap_pointers_start; /* Can hold 50k inodes assuming 1024 size */
    uint64_t block_bitmap_pointers_start; /* Can hold approx 1 mil blocks */
    uint64_t inode_start_pointer; /* Where inodes start */
    uint64_t block_start_pointer; /* Where blocks start */
    uint64_t reserved[116]; // just to keep the size nice, we can do something with this space later if we so choose
};

_Static_assert(sizeof(struct tempfs_superblock) == TEMPFS_BLOCKSIZE, "Tempfs Superblock not the proper size");

//4 inodes per block
struct tempfs_inode {
    uint16_t uid;
    uint16_t inode_number;
    uint16_t parent_inode_number;
    uint16_t type;
    uint16_t refcount;
    char name[MAX_FILENAME_LENGTH];
    uint32_t size; // For directories, we'll just add 1 for each dirent
    uint32_t block_count;
    uint64_t blocks[10]; /* Will point to logical block numbers */
    uint64_t single_indirect;
    uint64_t double_indirect;
    uint64_t triple_indirect;
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
    uint32_t inode_number;
    uint32_t parent_inode_number;
    uint16_t type;
    uint16_t device_number;
    uint32_t size;
};


struct tempfs_byte_offset_indices {
    uint16_t direct_block_number;
    uint16_t third_level_block_number;
    uint16_t second_level_block_number;
    uint16_t first_level_block_number;
    uint16_t block_number;
    uint16_t levels_indirection;
};

struct tempfs_filesystem {
    uint64_t filesystem_id;
    struct spinlock *lock;
    struct tempfs_superblock *superblock;
    uint64_t ramdisk_id;
};

_Static_assert(sizeof(struct tempfs_inode) % 256 == 0, "Tempfs inode not the proper size");


void tempfs_init(uint64_t filesystem_id);
void tempfs_mkfs(uint64_t ramdisk_id, struct tempfs_filesystem* fs);
uint64_t tempfs_read(struct vnode* vnode, uint64_t offset, uint8_t* buffer, uint64_t bytes);
uint64_t tempfs_write(struct vnode* vnode, uint64_t offset, uint8_t* buffer, uint64_t bytes);
uint64_t tempfs_stat(struct vnode* vnode, uint64_t offset, uint8_t* buffer, uint64_t bytes);
struct vnode* tempfs_lookup(struct vnode* vnode, char* name);
struct vnode* tempfs_create(struct vnode* parent, char *name, uint8_t vnode_type);
void tempfs_close(struct vnode* vnode);
struct vnode* tempfs_link(struct vnode* vnode, struct vnode* new_vnode);
void tempfs_unlink(struct vnode* vnode);
void tempfs_remove(struct vnode* vnode);
void tempfs_rename(struct vnode* vnode, char* new_name);
uint64_t tempfs_open(struct vnode* vnode);

