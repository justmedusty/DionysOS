//
// Created by dustyn on 9/18/24.
//

#include "include/data_structures/spinlock.h"
#include "include/definitions.h"
#include "include/filesystem/vfs.h"
#include "include/filesystem/tempfs.h"
#include <include/drivers/serial/uart.h>
#include <include/mem/kalloc.h>
#include "include/drivers/block/ramdisk.h"
#include "include/mem/mem.h"

struct spinlock tempfs_lock;
struct tempfs_superblock tempfs_superblock;

static uint64 tempfs_modify_bitmap(struct tempfs_superblock* sb, uint8 type, uint64 ramdisk_id, uint64 number,
                                   uint8 action);
static uint64 tempfs_get_inode(struct tempfs_superblock* sb, uint64 inode_number, uint64 ramdisk_id,
                               struct tempfs_inode* inode_to_be_filled);
static uint64 tempfs_get_free_inode_and_mark_bitmap(struct tempfs_superblock* sb, uint64 ramdisk_id);
static uint64 tempfs_get_free_block_and_mark_bitmap(struct tempfs_superblock* sb, uint64 ramdisk_id);
static uint64 tempfs_free_block_and_mark_bitmap(struct tempfs_superblock* sb, uint64 ramdisk_id, uint64 inode_number);
static uint64 tempfs_free_inode_and_mark_bitmap(struct tempfs_superblock* sb, uint64 ramdisk_id, uint64 inode_number);
static uint64 tempfs_get_bytes_from_inode(struct tempfs_superblock* sb, uint64 ramdisk_id, uint8* buffer,
                                          uint64 buffer_size, uint64 inode_number, uint64 byte_start,
                                          uint64 size_to_read);
static uint64 tempfs_get_directory_entries(struct tempfs_superblock* sb, uint64 ramdisk_id, uint8* buffer,
                                           struct tempfs_inode** children, uint64 inode_number);

struct vnode_operations tempfs_vnode_ops = {
    .lookup = tempfs_lookup,
    .create = tempfs_create,
    .read = tempfs_read,
    .write = tempfs_write,
    .open = tempfs_open,
    .close = tempfs_close,
    .remove = tempfs_remove,
    .rename = tempfs_rename,
    .link = tempfs_link,
    .unlink = tempfs_unlink,
};

/*
 * This function will do basic setup such as initing the tempfs lock, it will then setup the ramdisk driver and begin to fill ramdisk memory with a tempfs filesystem.
 * It will use the size DEFAULT_TEMPFS_SIZE and any other size, modifications need to be made to the superblock object.
 */
void tempfs_init() {
    initlock(&tempfs_lock,TEMPFS_LOCK);
    ramdisk_init(DEFAULT_TEMPFS_SIZE,INITRAMFS, "initramfs");
};

void tempfs_remove(struct vnode* vnode) {
}

void tempfs_rename(struct vnode* vnode, char* new_name) {
}

uint64 tempfs_read(struct vnode* vnode, uint64 offset, char* buffer, uint64 bytes) {
}

uint64 tempfs_write(struct vnode* vnode, uint64 offset, char* buffer, uint64 bytes) {
}

uint64 tempfs_stat(struct vnode* vnode, uint64 offset, char* buffer, uint64 bytes) {
}

struct vnode* tempfs_lookup(struct vnode* vnode, char* name) {
}

struct vnode* tempfs_create(struct vnode* vnode, struct vnode* new_vnode, uint8 vnode_type) {
}

void tempfs_close(struct vnode* vnode) {
}

uint64 tempfs_open(struct vnode* vnode) {
}

struct vnode* tempfs_link(struct vnode* vnode, struct vnode* new_vnode) {
}

void tempfs_unlink(struct vnode* vnode) {
}

/*
 * This will be the function that spins up an empty tempfs filesystem and then fill with a root directory and a few basic directories and files.
 *
 * Takes a ramdisk ID to specify which ramdisk to operate on
 */
void tempfs_mkfs(uint64 ramdisk_id) {
    uint8* buffer = kalloc(PAGE_SIZE);

    uint64 ret = ramdisk_read(buffer, 0, 0,TEMPFS_BLOCKSIZE,PAGE_SIZE, ramdisk_id);

    if (ret != SUCCESS) {
        HANDLE_RAMDISK_ERROR(ret, "tempfs_mkfs")
        return;
    }

    tempfs_superblock = *(struct tempfs_superblock*)buffer;
    tempfs_superblock.magic = TEMPFS_MAGIC;
    tempfs_superblock.version = TEMPFS_VERSION;
    tempfs_superblock.block_size = TEMPFS_BLOCKSIZE;
    tempfs_superblock.num_blocks = ((sizeof(tempfs_superblock.block_bitmap_pointers) / 8) * TEMPFS_BLOCKSIZE) * 8;
    tempfs_superblock.num_inodes = ((sizeof(tempfs_superblock.inode_bitmap_pointers) / 8) * TEMPFS_BLOCKSIZE) * 8;
    tempfs_superblock.inode_start_pointer = TEMPFS_START_INODES;
    tempfs_superblock.block_start_pointer = TEMPFS_START_BLOCKS;

    for (uint64 i = 0; i < TEMPFS_NUM_INODE_POINTER_BLOCKS; i++) {
        tempfs_superblock.inode_bitmap_pointers[i] = TEMPFS_START_INODE_BITMAP + i;
    }

    for (uint64 i = 0; i < TEMPFS_NUM_BLOCK_POINTER_BLOCKS; i++) {
        tempfs_superblock.block_bitmap_pointers[i] = TEMPFS_START_BLOCK_BITMAP + i;
    }

    tempfs_superblock.total_size = DEFAULT_TEMPFS_SIZE;

    //copy the contents into our buffer
    memcpy(buffer, &tempfs_superblock, sizeof(struct tempfs_superblock));

    //Write the new superblock to the ramdisk
    ramdisk_write(buffer,TEMPFS_SUPERBLOCK, 0,TEMPFS_BLOCKSIZE,PAGE_SIZE, ramdisk_id);

    memset(buffer, 0,PAGE_SIZE);

    /*
     * Fill in inode bitmap with zeros blocks
     */
    ramdisk_write(buffer,TEMPFS_START_INODE_BITMAP, 0,TEMPFS_NUM_INODE_POINTER_BLOCKS * TEMPFS_BLOCKSIZE,
                  TEMPFS_BLOCKSIZE,INITRAMFS);

    /*
     * Fill in block bitmap with zerod blocks
     */
    ramdisk_write(buffer,TEMPFS_START_BLOCK_BITMAP, 0,TEMPFS_NUM_BLOCK_POINTER_BLOCKS * TEMPFS_BLOCKSIZE,
                  TEMPFS_BLOCKSIZE,INITRAMFS);

    /*
     *Write zerod blocks for the inode blocks
     */
    ramdisk_write(buffer,TEMPFS_START_INODES, 0,TEMPFS_NUM_INODES * TEMPFS_BLOCKSIZE,TEMPFS_BLOCKSIZE,INITRAMFS);
    /*
     *Write zerod blocks for the blocks
     */
    ramdisk_write(buffer,TEMPFS_START_BLOCKS, 0, TEMPFS_NUM_BLOCKS * TEMPFS_BLOCKSIZE,TEMPFS_BLOCKSIZE,INITRAMFS);
}


/*
 *  These are all fairly self-explanatory internal helper functions for doing things such a reading and setting bitmaps,
 *  following block pointers to get data, getting specific bytes from a file and moving to a buffer, getting directory entries,
 *  and so on.
 *
 *  These will be hidden away since the implementation is ugly with a lot of calculation and iteration, they are meant to be used
 *  in conjunction with each other in specific ways so they will not be linked outside of this file.
 */


/*
 * type is passed as the macro pair BITMAP_TYPE_BLOCK or BITMAP_TYPE_INODE
 * number is the block or inode # so that its spot can be calculated
 * action is the macros TEMPFS_TYPE_SET or TEMPFS_TYPE_CLEAR
 */
static uint64 tempfs_modify_bitmap(struct tempfs_superblock* sb, uint8 type, uint64 ramdisk_id, uint64 number,
                                   uint8 action) {
    if (type == BITMAP_TYPE_BLOCK) {
        uint64 block_to_read = sb->block_bitmap_pointers[number / (TEMPFS_BLOCKSIZE * 8)];

        uint64 block = sb->block_bitmap_pointers[block_to_read];

        uint64 byte_in_block = number / 8;

        uint64 bit = number % 8;

        uint8* buffer = kalloc(PAGE_SIZE);
        uint64 ret = ramdisk_read(buffer, block, 0, TEMPFS_BLOCKSIZE,PAGE_SIZE, ramdisk_id);

        if (ret != SUCCESS) {
            HANDLE_RAMDISK_ERROR(ret, "tempfs_modify_bitmap read")
            return ret;
        }

        buffer[byte_in_block] = buffer[byte_in_block] & (action << bit); /* 0 the bit and write it back */

        ret = ramdisk_write(buffer, block, 0, TEMPFS_BLOCKSIZE,PAGE_SIZE, ramdisk_id);

        if (ret != SUCCESS) {
            HANDLE_RAMDISK_ERROR(ret, "tempfs_modify_bitmap read")
            return ret;
        }

        return SUCCESS;
    }

    if (type == BITMAP_TYPE_INODE) {
        uint64 block_to_read = sb->inode_bitmap_pointers[number / (TEMPFS_BLOCKSIZE * 8)];

        uint64 block = sb->inode_bitmap_pointers[block_to_read];

        uint64 byte_in_block = number / 8;

        uint64 bit = number % 8;

        uint8* buffer = kalloc(PAGE_SIZE);
        uint64 ret = ramdisk_read(buffer, block, 0, TEMPFS_BLOCKSIZE,PAGE_SIZE, ramdisk_id);

        if (ret != SUCCESS) {
            HANDLE_RAMDISK_ERROR(ret, "tempfs_modify_bitmap read")
            return ret;
        }

        buffer[byte_in_block] = buffer[byte_in_block] & (action << bit); /* 0 the bit and write it back */

        ret = ramdisk_write(buffer, block, 0, TEMPFS_BLOCKSIZE,PAGE_SIZE, ramdisk_id);

        if (ret != SUCCESS) {
            HANDLE_RAMDISK_ERROR(ret, "tempfs_modify_bitmap read")
            return ret;
        }

        return SUCCESS;
    }

    panic("tempfs_modify_bitmap unknown type"); /* Dramatic but it's okay this shouldn't happen anyway */
}

/*
 *  Reads inode at place inode_number on ramdisk of ramdisk_id and fills the passed inode_to_be_filled with the ramdisk inode
 */
static uint64 tempfs_get_inode(struct tempfs_superblock* sb, uint64 inode_number, uint64 ramdisk_id,
                               struct tempfs_inode* inode_to_be_filled) {
    uint64 inode_block = sb->inode_start_pointer + (sb->block_size / TEMPFS_INODE_SIZE);
    uint64 inode_in_block = inode_number % (sb->block_size / TEMPFS_INODE_SIZE);

    uint8* block = kalloc(PAGE_SIZE);
    /* I will need to add the tempfs block size to the heap until then I'll just allocate a page*/

    uint64 ret = ramdisk_read(block, inode_block, inode_in_block * TEMPFS_INODE_SIZE,TEMPFS_INODE_SIZE,PAGE_SIZE,
                              ramdisk_id);

    if (ret == SUCCESS) {
        *inode_to_be_filled = *(struct tempfs_inode*)block;
        return SUCCESS;
    }

    HANDLE_RAMDISK_ERROR(ret, "tempfs_get_inode")
    return ret;
}

static uint64 tempfs_get_free_inode_and_mark_bitmap(struct tempfs_superblock* sb, uint64 ramdisk_id) {
}

static uint64 tempfs_get_free_block_and_mark_bitmap(struct tempfs_superblock* sb, uint64 ramdisk_id) {
}

static uint64 tempfs_free_block_and_mark_bitmap(struct tempfs_superblock* sb, uint64 ramdisk_id, uint64 inode_number) {
}

static uint64 tempfs_free_inode_and_mark_bitmap(struct tempfs_superblock* sb, uint64 ramdisk_id, uint64 inode_number) {
}

static uint64 tempfs_get_bytes_from_inode(struct tempfs_superblock* sb, uint64 ramdisk_id, uint8* buffer,
                                          uint64 buffer_size, uint64 inode_number, uint64 byte_start,
                                          uint64 size_to_read) {
}


static uint64 tempfs_get_directory_entries(struct tempfs_superblock* sb, uint64 ramdisk_id, uint8* buffer,
                                           struct tempfs_inode** children, uint64 inode_number) {
}

