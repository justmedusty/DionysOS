//
// Created by dustyn on 9/17/24.
//

#pragma once
#include "include/types.h"
#include "include/data_structures/spinlock.h"

#define SUCCESS 0
#define RAMDISK_SIZE_TOO_SMALL 0x4
#define RAMDISK_ID_OUT_OF_RANGE 0x8

#define DEFAULT_RAMDISK_SIZE (0xFF * PAGE_SIZE)
#define RAMDISK_TEMPFS_ID 0x1
#define RAMDISK_EXT2_ID 0x2


struct ramdisk {
    void *ramdisk_start;
    void *ramdisk_end;
    uint64 ramdisk_size_pages;
    uint64 block_size;
    struct spinlock ramdisk_lock;
    uint16 file_system_id;
    char ramdisk_name[32];
};


void ramdisk_init(const uint64 size,const uint64 ramdisk_id);
uint64 ramdisk_mkfs(const int8 *initramfs_img,const uint64 size_bytes, const uint64 ramdisk_id);
void ramdisk_destroy(const uint64 ramdisk_id);
uint64 ramdisk_read(uint8 *buffer, uint64 block, uint64 offset, uint64 read_size,uint64 buffer_size);
uint64 ramdisk_write(uint8 *buffer, uint64 block, uint64 offset, uint64 write_size,uint64 buffer_size);
