//
// Created by dustyn on 9/17/24.
//

#include "ramdisk.h"
#include <include/types.h>
#include <include/mem/kalloc.h>

#include "include/mem/pmm.h"

#define DEFAULT_RAMDISK_SIZE (0xFF * PAGE_SIZE)
static uint64 ramdisk_start_address;
static uint64 ramdisk_end_address;
static uint64 ramdisk_size;

//This probably needs to be made virtually contiguous
void ramdisk_init(uint64 pages) {
        if (pages == 0) {
                pages = DEFAULT_RAMDISK_SIZE;
        }
        ramdisk_start_address = (uint64) kalloc(pages);
        ramdisk_size = pages;
        ramdisk_end_address = ramdisk_start_address + (pages * 0x1000);
}

void ramdisk_mkfs(char *initramfs_img) {

}

void ramdisk_free() {

}

uint64 ramdisk_read(char *buffer, uint64 block, uint64 offset, uint64 size) {
}

uint64 ramdisk_write(char *buffer, uint64 block, uint64 offset, uint64 size) {
}



