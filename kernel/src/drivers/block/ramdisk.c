//
// Created by dustyn on 9/17/24.
//

#include "include/drivers/block/ramdisk.h"
#include <include/types.h>
#include <include/drivers/serial/uart.h>
#include <include/filesystem/diosfs.h>
#include <include/mem/kalloc.h>
#include "include/mem/mem.h"
#include "include/mem/pmm.h"
#include "include/definitions.h"
#include "include/definitions/string.h"

struct ramdisk ramdisk[RAMDISK_COUNT]; /* Only need one but why not 3! */
uint64_t ramdisk_count = RAMDISK_COUNT;

/*
 * This function initializes a ramdisk of size bytes converted to pages, ramdisk id which is just the index into the array, and a string which could be useful at some point.
 * It initializes the lock, page count, allocates memory yada yada yada
 */
void ramdisk_init(uint64_t size_bytes, const uint64_t ramdisk_id, char* name, uint64_t block_size) {
        if (ramdisk_id > ramdisk_count) {
                serial_printf("ramdisk id is out of range\n");
                return;
        }
        if (size_bytes == 0) {
                size_bytes = DEFAULT_RAMDISK_SIZE;
        }

        ramdisk[ramdisk_id].ramdisk_start =kmalloc(size_bytes);
        ramdisk[ramdisk_id].ramdisk_size_pages = size_bytes / PAGE_SIZE;
        ramdisk[ramdisk_id].ramdisk_end = ramdisk[ramdisk_id].ramdisk_start + (ramdisk[ramdisk_id].ramdisk_size_pages *
                PAGE_SIZE);
        ramdisk[ramdisk_id].block_size = block_size;
        safe_strcpy(ramdisk[ramdisk_id].ramdisk_name, name, sizeof(ramdisk[ramdisk_id].ramdisk_name));
        initlock(&ramdisk[ramdisk_id].ramdisk_lock, RAMDISK_LOCK);
        serial_printf("Ramdisk initialized\n");
}

/*
 * This function will read a filesystem image and copy it into the ramdisk memory to get the party started
 */
uint64_t ramdisk_mkfs(const char* initramfs_img, const uint64_t size_bytes, const uint64_t ramdisk_id) {
        if (ramdisk_id > ramdisk_count) {
                return RAMDISK_ID_OUT_OF_RANGE;
        }
        if ((size_bytes / PAGE_SIZE) < ramdisk[ramdisk_id].ramdisk_size_pages) {
                return RAMDISK_SIZE_TOO_SMALL;
        }
        memcpy(ramdisk[ramdisk_id].ramdisk_start, initramfs_img, size_bytes);
        return SUCCESS;
}

/*
 * I doubt I'll ever even use this but this is for deallocating a ramdisk, obviously
 */
void ramdisk_destroy(const uint64_t ramdisk_id) {
        if (ramdisk_id > ramdisk_count) {
                serial_printf("ramdisk id is out of range\n");
                return;
        }
        kfree(ramdisk[ramdisk_id].ramdisk_start);
        ramdisk[ramdisk_id].ramdisk_start = NULL;
        ramdisk[ramdisk_id].ramdisk_size_pages = 0;
        ramdisk[ramdisk_id].ramdisk_end = NULL;
}

/*
 * We will just assume diosfs for now, but we can add support for other file systems in the future
 * This function will just read a block, offset, into buffer of buffer size until either read_size or buffer_size is hit.
 *
 * We will act as though the buffer is always empty.
 */
uint64_t ramdisk_read(char* buffer, uint64_t block, uint64_t offset, uint64_t read_size, uint64_t buffer_size,
                      uint64_t ramdisk_id) {
        if (ramdisk_id > ramdisk_count) {
                return RAMDISK_ID_OUT_OF_RANGE;
        }

        if (offset > ramdisk[ramdisk_id].block_size) {
                return RAMDISK_OFFSET_OUT_OF_RANGE;
        }

        if (read_size > buffer_size) {
                return RAMDISK_READ_SIZE_OUT_OF_BOUNDS;
        }

        if (block > ((ramdisk[ramdisk_id].ramdisk_size_pages * PAGE_SIZE) / ramdisk[ramdisk_id].block_size)) {
                return BLOCK_OUT_OF_RANGE;
        }

        char* read_start = ramdisk[ramdisk_id].ramdisk_start + (block * ramdisk[ramdisk_id].block_size) + offset;

        uint64_t index = 0;

        while (index < read_size) {
                buffer[index] = read_start[index];
                index++;
        }
        return SUCCESS;
}

/*
 * We will just assume diosfs for now, but we can add support for other file systems in the future
 * This function will just write a block, offset, from buffer of buffer size until either write_size or buffer_size is hit.
 */
uint64_t ramdisk_write(const char* buffer, uint64_t block, uint64_t offset, uint64_t write_size,
                       uint64_t buffer_size,
                       uint64_t ramdisk_id) {


        if (ramdisk_id > ramdisk_count) {
                return RAMDISK_ID_OUT_OF_RANGE;
        }

        if (offset > ramdisk[ramdisk_id].block_size) {
                return RAMDISK_OFFSET_OUT_OF_RANGE;
        }
 if (block > ((ramdisk[ramdisk_id].ramdisk_size_pages * PAGE_SIZE) / ramdisk[ramdisk_id].block_size)) {
         serial_printf("Block too large %i %i\n",block,buffer_size);
                return BLOCK_OUT_OF_RANGE;
        }

        /*
         * We will make an exception for write size exceeding buffer in the case of a block being written many times over since this makes more
         * sense performance wise than making an enormous buffer just to write zeros of special crafted blocks ad nauseam
         */
        if (write_size > buffer_size && write_size % buffer_size == 0) {
                uint64_t index = 0;
                uint64_t total_transferred = 0;

                char* read_start = ramdisk[ramdisk_id].ramdisk_start + (block * ramdisk[ramdisk_id].block_size) +
                        offset;

                while (total_transferred < write_size) {
                        read_start[index] = buffer[index];
                        total_transferred++;

                        if (index == buffer_size) {
                                index = 0;
                        }

                        index++;
                        total_transferred++;
                }
                return SUCCESS;
        }

        if (write_size > buffer_size) {
                return RAMDISK_READ_SIZE_OUT_OF_BOUNDS;
        }


        char* read_start = ramdisk[ramdisk_id].ramdisk_start + ((block * ramdisk[ramdisk_id].block_size) + offset);



        uint64_t index = 0;

        while (index < write_size) {
                read_start[index] = buffer[index];
                index++;
        }

        return SUCCESS;
}



