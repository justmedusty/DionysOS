//
// Created by dustyn on 9/17/24.
//

#include "ramdisk.h"
#include <include/types.h>
#include "include/mem/pmm.h"

static uint64 ramdisk_start_address;
static uint64 ramdisk_end_address;
static uint64 ramdisk_size;


void ramdisk_init(uint64 pages) {
        ramdisk_start_address = (uint64) phys_alloc(pages);
        ramdisk_size = pages;
        ramdisk_end_address = ramdisk_start_address + (pages * 0x1000);
}

void ramdisk_mkfs(uint64 filesystem_type, uint64 pages, char *initramfs_img) {
}

void ramdisk_free() {
}

uint64 ramdisk_read(char *buffer, uint64 block, uint64 offset, uint64 size) {
}

uint64 ramdisk_write(char *buffer, uint64 block, uint64 offset, uint64 size) {
}


