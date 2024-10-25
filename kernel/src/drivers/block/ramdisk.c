//
// Created by dustyn on 9/17/24.
//

#include "include/filesystem/ramdisk.h"
#include <include/types.h>
#include <include/filesystem/tempfs.h>
#include <include/mem/kalloc.h>
#include "include/mem/mem.h"
#include "include/mem/pmm.h"

#define DEFAULT_RAMDISK_SIZE (0xFF * PAGE_SIZE)
static void *ramdisk_start_address;
static void *ramdisk_end_address;
static uint64 ramdisk_size;


void ramdisk_init(uint64 size) {
        uint64 pages = size / PAGE_SIZE;
        if (pages == 0) {
                pages = DEFAULT_RAMDISK_SIZE;
        }
        ramdisk_start_address = kalloc(pages);
        ramdisk_size = pages;
        ramdisk_end_address = ramdisk_start_address + (pages * PAGE_SIZE);
}

uint64 ramdisk_mkfs(int8 *initramfs_img,uint64 size_bytes) {
        if(size_bytes < ramdisk_size) {
                return RAMDISK_SIZE_TOO_SMALL;
        }
        memcpy(ramdisk_start_address, initramfs_img, size_bytes);
        return SUCCESS;
}

void ramdisk_destroy() {
        kfree(ramdisk_start_address);
        ramdisk_start_address = NULL;
        ramdisk_size = 0;
        ramdisk_end_address = NULL;
}

/*
 * We will just assume tempfs for now, but we can add support for other file systems in the future
 */
uint64 ramdisk_read(uint8 *buffer, uint64 block, uint64 offset, uint64 read_size,uint64 buffer_size) {
}

uint64 ramdisk_write(uint8 *buffer, uint64 block, uint64 offset, uint64 write_size,uint64 buffer_size) {
}



