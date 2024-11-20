//
// Created by dustyn on 7/6/24.
//

#ifndef KERNEL_KALLOC_H
#define KERNEL_KALLOC_H

void *kalloc(uint64_t size);
void *krealloc(void *address, uint64_t new_size);
void kfree(void *address);

#endif //KERNEL_KALLOC_H
