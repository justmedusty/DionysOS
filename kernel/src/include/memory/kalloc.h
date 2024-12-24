//
// Created by dustyn on 7/6/24.
//

#ifndef KERNEL_KALLOC_H
#define KERNEL_KALLOC_H
#include <stdint.h>
#include <stddef.h>
void *kmalloc(uint64_t size);
void *_kalloc(uint64_t size);
void *krealloc(void *address, uint64_t new_size);
void _kfree(void *address);
void kfree(void *address);
void *kzmalloc(uint64_t size);
#endif //KERNEL_KALLOC_H
