//
// Created by dustyn on 9/17/24.
//

#include "include/filesystem/ramdisk.h"
#include <include/types.h>
#include <include/drivers/uart.h>
#include <include/filesystem/tempfs.h>
#include <include/mem/kalloc.h>
#include "include/mem/mem.h"
#include "include/mem/pmm.h"

struct ramdisk ramdisk[3]; /* 3 Only need one but why not 3! */
uint64 ramdisk_count = sizeof(ramdisk) / sizeof(ramdisk[0]);

void ramdisk_init(const uint64 size,const uint64 ramdisk_id) {

        if(ramdisk_id > ramdisk_count) {
                serial_printf("ramdisk id is out of range\n");
                return;
        }
        uint64 pages = size / PAGE_SIZE;
        if (pages == 0) {
                pages = DEFAULT_RAMDISK_SIZE;
        }
        ramdisk[ramdisk_id].ramdisk_start = kalloc(pages);
         ramdisk[ramdisk_id].ramdisk_size_pages = pages;
         ramdisk[ramdisk_id].ramdisk_end =  ramdisk[ramdisk_id].ramdisk_start + (pages * PAGE_SIZE);
}

uint64 ramdisk_mkfs(const int8 *initramfs_img,const uint64 size_bytes, const uint64 ramdisk_id) {
        if(ramdisk_id > ramdisk_count) {
                return RAMDISK_ID_OUT_OF_RANGE;
        }
        if((size_bytes / PAGE_SIZE) < ramdisk[ramdisk_id].ramdisk_size_pages) {
                return RAMDISK_SIZE_TOO_SMALL;
        }
        memcpy(ramdisk[ramdisk_id].ramdisk_start, initramfs_img, size_bytes);
        return SUCCESS;
}

void ramdisk_destroy(const uint64 ramdisk_id) {
        if(ramdisk_id > ramdisk_count) {
                serial_printf("ramdisk id is out of range\n");
                return;
        }
        kfree(ramdisk[ramdisk_id].ramdisk_start);
        ramdisk[ramdisk_id].ramdisk_start = NULL;
        ramdisk[ramdisk_id].ramdisk_size_pages = 0;
        ramdisk[ramdisk_id].ramdisk_end = NULL;
}

/*
 * We will just assume tempfs for now, but we can add support for other file systems in the future
 */
uint64 ramdisk_read(uint8 *buffer, uint64 block, uint64 offset, uint64 read_size,uint64 buffer_size) {
}

uint64 ramdisk_write(uint8 *buffer, uint64 block, uint64 offset, uint64 write_size,uint64 buffer_size) {
}



