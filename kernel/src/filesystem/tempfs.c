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
 */
void tempfs_mkfs(uint64 ramdisk_id) {
    uint8 *buffer = kalloc(PAGE_SIZE);

    uint64 ret = ramdisk_read(buffer, 0,0,TEMPFS_BLOCKSIZE,PAGE_SIZE,ramdisk_id);

    switch (ret) {
    case SUCCESS:
        break;
    case RAMDISK_SIZE_TOO_SMALL:
        serial_printf("Ramdisk size too small\n");
        break;
    case RAMDISK_ID_OUT_OF_RANGE:
        serial_printf("RAMDISK ID OUT OF RANGE\n");
        break;
    case RAMDISK_BLOCK_OUT_OF_RANGE:
        serial_printf("RAMDISK BLOCK OUT OF RANGE\n");
        break;
    case RAMDISK_OFFSET_OUT_OF_RANGE:
        serial_printf("RAMDISK OFFSET OUT OF RANGE\n");
        break;
    case RAMDISK_READ_SIZE_OUT_OF_BOUNDS:
        serial_printf("RAMDISK READ SIZE OUT OF BOUNDS\n");
        break;
    case BLOCK_OUT_OF_RANGE:
        serial_printf("BLOCK OUT OF RANGE\n");
        break;
    default:
        panic("An unexpected result was returned from ramdisk_read");
    }


    tempfs_superblock = *(struct tempfs_superblock *) buffer;
    tempfs_superblock.magic = TEMPFS_MAGIC;
    tempfs_superblock.version = TEMPFS_VERSION;
    tempfs_superblock.block_size = TEMPFS_BLOCKSIZE;
    tempfs_superblock.num_blocks = ((sizeof(tempfs_superblock.block_bitmap_pointers) / 8 ) * TEMPFS_BLOCKSIZE) * 8;
    tempfs_superblock.num_inodes = ((sizeof(tempfs_superblock.inode_bitmap_pointers) / 8 ) * TEMPFS_BLOCKSIZE) * 8;
    tempfs_superblock.inode_start_pointer = TEMPFS_START_INODES;
    tempfs_superblock.block_start_pointer = TEMPFS_START_BLOCKS;

    for(uint64 i = 0; i < TEMPFS_NUM_INODE_POINTER_BLOCKS;i++) {
        tempfs_superblock.inode_bitmap_pointers[i] = TEMPFS_START_INODE_BITMAP + i;
    }

    for(uint64 i = 0; i < TEMPFS_NUM_BLOCK_POINTER_BLOCKS;i++) {
        tempfs_superblock.block_bitmap_pointers[i] = TEMPFS_START_BLOCK_BITMAP + i;
    }

    tempfs_superblock.total_size = DEFAULT_TEMPFS_SIZE;

    //copy the contents into our buffer
    memcpy(buffer, &tempfs_superblock, sizeof(struct tempfs_superblock));

    //Write the new superblock to the ramdisk
    ramdisk_write(buffer,TEMPFS_SUPERBLOCK,0,TEMPFS_BLOCKSIZE,PAGE_SIZE,ramdisk_id);

    memset(buffer,0,PAGE_SIZE);








}