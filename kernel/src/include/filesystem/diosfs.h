//
// Created by dustyn on 9/18/24.
//

#pragma once
#include "include/types.h"
#include "stdint.h"
#include "include/filesystem/vfs.h"
#define INITRAMFS 0 /* Just for ramdisk 0 id purposes*/

#define DIOSFS_BLOCKSIZE 1024
#define DIOSFS_MAGIC 0x7777777777777777
#define DIOSFS_VERSION 1
#define MAX_FILENAME_LENGTH 128 /* This number is here so we can fit 2 inodes in 1 2048 block */

#define DEFAULT_DIOSFS_SIZE (DIOSFS_BLOCKSIZE /* super block */ + (DIOSFS_NUM_INODE_POINTER_BLOCKS * DIOSFS_BLOCKSIZE) + (DIOSFS_NUM_BLOCK_POINTER_BLOCKS  * DIOSFS_BLOCKSIZE) + (DIOSFS_NUM_BLOCKS  * DIOSFS_BLOCKSIZE) + (DIOSFS_NUM_INODES  * DIOSFS_BLOCKSIZE))
#define MAX_BLOCKS_IN_INODE (((((NUM_BLOCKS_DIRECT) * DIOSFS_BLOCKSIZE) * NUM_BLOCKS_IN_INDIRECTION_BLOCK) * NUM_BLOCKS_IN_INDIRECTION_BLOCK) * NUM_BLOCKS_IN_INDIRECTION_BLOCK)

#define NUM_BLOCKS_IN_INDIRECTION_BLOCK ((DIOSFS_BLOCKSIZE / sizeof(uint64_t)))

#define DIOSFS_DIRECTORY 0
#define DIOSFS_REG_FILE 1
#define DIOSFS_SYMLINK 2

#define DIOSFS_NUM_INODE_POINTER_BLOCKS 16
#define DIOSFS_NUM_BLOCK_POINTER_BLOCKS 128

#define DIOSFS_NUM_BLOCKS (DIOSFS_NUM_BLOCK_POINTER_BLOCKS * DIOSFS_BLOCKSIZE * 8)
#define DIOSFS_NUM_INODES (DIOSFS_NUM_INODE_POINTER_BLOCKS * DIOSFS_BLOCKSIZE * 8)

#define NUM_INODES_PER_BLOCK ((DIOSFS_BLOCKSIZE / sizeof(struct diosfs_inode)))
#define DIOSFS_INODES_PER_BLOCK (DIOSFS_BLOCKSIZE / DIOSFS_INODE_SIZE)
#define DIOSFS_BLOCK_POINTER_SIZE sizeof(uint64_t)

#define BITMAP_TYPE_BLOCK 0
#define BITMAP_TYPE_INODE 1

#define BITMAP_ACTION_SET 1
#define BITMAP_ACTION_CLEAR 0

#define NUM_BLOCKS_DIRECT 10
#define NUM_BLOCKS_SINGULAR_INDIRECTION (NUM_BLOCKS_IN_INDIRECTION_BLOCK)
#define NUM_BLOCKS_DOUBLE_INDIRECTION (NUM_BLOCKS_IN_INDIRECTION_BLOCK * NUM_BLOCKS_IN_INDIRECTION_BLOCK)
#define NUM_BLOCKS_TRIPLE_INDIRECTION (NUM_BLOCKS_IN_INDIRECTION_BLOCK * NUM_BLOCKS_IN_INDIRECTION_BLOCK * NUM_BLOCKS_IN_INDIRECTION_BLOCK)

#define DIOSFS_INODE_SIZE sizeof(struct diosfs_inode)
#define DIOSFS_NUM_INODES DIOSFS_NUM_INODE_POINTER_BLOCKS * DIOSFS_BLOCKSIZE * 8
#define DIOSFS_NUM_BLOCKS DIOSFS_NUM_BLOCK_POINTER_BLOCKS * DIOSFS_BLOCKSIZE * 8

#define DIOSFS_BLOCK_UNALLOCATED 0xFFFFFFFFFFFFFFFF
#define DIOSFS_BUFFER_TOO_SMALL 0x1
#define DIOSFS_NOT_A_DIRECTORY 0x2
#define DIOSFS_CANT_ALLOCATE_BLOCKS_FOR_DIR 0x3
#define CANNOT_WRITE_DIRECTORY 0x4
#define BAD_SYMLINK 0x5
#define UNEXPECTED_SYMLINK_TYPE 0x6

#define DIOSFS_ERROR 0x6
#define SUCCESS 0
#define NOT_FOUND 1

#define DIOSFS_MAX_FILES_IN_DIRENT_BLOCK ((fs->superblock->block_size / sizeof(struct diosfs_directory_entry)))
#define DIOSFS_MAX_FILES_IN_DIRECTORY ((NUM_BLOCKS_DIRECT * fs->superblock->block_size) / sizeof(struct diosfs_directory_entry))
/*
 *  These macros make it easier to change the size created by diosfs_init by just modifying values of
 *  DIOSFS_NUM_INODE_POINTER_BLOCKS and DIOSFS_NUM_BLOCK_POINTER_BLOCKS
 */
#define DIOSFS_SUPERBLOCK 0
#define DIOSFS_START_INODE_BITMAP 1
#define DIOSFS_START_BLOCK_BITMAP (DIOSFS_START_INODE_BITMAP + DIOSFS_NUM_INODE_POINTER_BLOCKS)
#define DIOSFS_START_INODES (DIOSFS_START_BLOCK_BITMAP + DIOSFS_NUM_BLOCK_POINTER_BLOCKS)
#define DIOSFS_START_BLOCKS (DIOSFS_START_INODES + ((DIOSFS_NUM_INODE_POINTER_BLOCKS * DIOSFS_BLOCKSIZE * 8) / NUM_INODES_PER_BLOCK))

#define DIOSFS_GET_BLOCK_NUMBER_OF_INODE(inode_number)
extern struct vnode_operations diosfs_vnode_ops;

struct diosfs_superblock {
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

_Static_assert(sizeof(struct diosfs_superblock) == DIOSFS_BLOCKSIZE, "Tempfs Superblock not the proper size");

//4 inodes per block
struct diosfs_inode {
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
 * because 50-70 dirents in one inode is more than enough for us. We want to keep things
 * simple after all.
 *
 * As it stands right now, 7 diosfs_dir entries will fit into a block, * 10 means
 * that a directory can hold 91 entries. More than enough.
 */
struct diosfs_directory_entry {
    char name[MAX_FILENAME_LENGTH];
    uint32_t inode_number;
    uint32_t parent_inode_number;
    uint16_t type;
    uint16_t device_number;
    uint32_t size;
};

/*
 * Returned for a calculation of indices for a given relative block number
 */
struct diosfs_byte_offset_indices {
    uint16_t direct_block_number;
    uint16_t third_level_block_number;
    uint16_t second_level_block_number;
    uint16_t first_level_block_number;
    uint16_t block_number;
    uint16_t levels_indirection;
};

struct diosfs_filesystem {
    uint64_t filesystem_id;
    struct spinlock *lock;
    struct diosfs_superblock *superblock;
    uint64_t ramdisk_id;
};


struct diosfs_symlink {
    char path[MAX_FILENAME_LENGTH * 8];
};
_Static_assert(sizeof(struct diosfs_inode) % 256 == 0, "Tempfs inode not the proper size");


void diosfs_init(uint64_t filesystem_id);
void diosfs_mkfs(uint64_t ramdisk_id, struct diosfs_filesystem* fs);
uint64_t diosfs_read(struct vnode* vnode, uint64_t offset, char* buffer, uint64_t bytes);
uint64_t diosfs_write(struct vnode* vnode, uint64_t offset, char* buffer, uint64_t bytes);
uint64_t diosfs_stat(const struct vnode* vnode);
struct vnode* diosfs_lookup(struct vnode* parent, char* name);
struct vnode* diosfs_create(struct vnode* parent, char *name, uint8_t vnode_type);
void diosfs_close(struct vnode* vnode,uint64_t handle);
struct vnode* diosfs_link(struct vnode* vnode, struct vnode* new_vnode,uint8_t type);
void diosfs_unlink(struct vnode* vnode);
void diosfs_remove(const struct vnode* vnode);
void diosfs_rename(const struct vnode* vnode, char* new_name);
uint64_t diosfs_open(struct vnode* vnode);

