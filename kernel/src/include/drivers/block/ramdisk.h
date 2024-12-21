//
// Created by dustyn on 9/17/24.
//
#ifndef _RAMDISK_H_
#define _RAMDISK_H_
#pragma once
#include "include/filesystem/diosfs.h"
#include <stdint.h>
#include <stddef.h>
#include "include/dev/device.h"
#include "include/types.h"
#include "include/data_structures/spinlock.h"

#define RAMDISK_COUNT 5

#define SUCCESS 0

#define SIZE_TOO_SMALL 0x4
#define ID_OUT_OF_RANGE 0x8
#define OFFSET_OUT_OF_RANGE 0x20
#define READ_SIZE_OUT_OF_BOUNDS 0x30
#define BLOCK_OUT_OF_RANGE 0x40

#define DEFAULT_RAMDISK_SIZE (0xFF * PAGE_SIZE)


extern struct device_ops ramdisk_device_ops;

/*
 * error handling very simple just prints a message
 * takes the return value and a string for identifying where the message is being printed
 */
#define HANDLE_DISK_ERROR(ret,string)               \
switch (ret) {                              \
case SIZE_TOO_SMALL:           \
serial_printf("Ramdisk size too small %s\n",string); \
break;                              \
case ID_OUT_OF_RANGE:          \
serial_printf("RAMDISK ID OUT OF RANGE %s \n",string); \
break;                              \
case OFFSET_OUT_OF_RANGE:      \
serial_printf("RAMDISK OFFSET OUT OF RANGE %s\n",string); \
break;                              \
case READ_SIZE_OUT_OF_BOUNDS:  \
serial_printf("RAMDISK READ SIZE OUT OF BOUNDS %s\n",string); \
break;                              \
case BLOCK_OUT_OF_RANGE:                \
serial_printf("BLOCK OUT OF RANGE %s\n",string); \
break;                              \
default:                                \
panic("An unexpected result was returned from block driver"); \
}


struct ramdisk {
    void* ramdisk_start;
    void* ramdisk_end;
    uint64_t ramdisk_size_pages;
    uint64_t block_size;
    struct spinlock ramdisk_lock;
    uint16_t file_system_id;
    char ramdisk_name[32];
};


extern struct ramdisk ramdisk[RAMDISK_COUNT];

void ramdisk_init(uint64_t size_bytes, uint64_t ramdisk_id, char* name,uint64_t block_size);
uint64_t ramdisk_mkfs(const char* initramfs_img, uint64_t size_bytes, uint64_t ramdisk_id);
void ramdisk_destroy(uint64_t ramdisk_id);
uint64_t ramdisk_read(char* buffer, uint64_t block, uint64_t offset, uint64_t read_size, uint64_t buffer_size,
                    uint64_t ramdisk_id);
uint64_t ramdisk_write(const char* buffer, uint64_t block, uint64_t offset, uint64_t write_size, uint64_t buffer_size,
                     uint64_t ramdisk_id);
#endif