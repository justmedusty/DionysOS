//
// Created by dustyn on 9/17/24.
//

#pragma once
#include "include/types.h"

#define SUCCESS 0
#define RAMDISK_SIZE_TOO_SMALL 0x4


void ramdisk_init(uint64 size);
uint64 ramdisk_mkfs(int8 *initramfs_img,uint64 size_bytes);
void ramdisk_destroy();
uint64 ramdisk_read(uint8 *buffer, uint64 block, uint64 offset, uint64 size);
uint64 ramdisk_write(uint8 *buffer, uint64 block, uint64 offset, uint64 size);