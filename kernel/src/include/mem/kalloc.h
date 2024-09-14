//
// Created by dustyn on 7/6/24.
//

#ifndef KERNEL_KALLOC_H
#define KERNEL_KALLOC_H

void *kalloc(uint64 size);
void *krealloc(void *address, uint64 new_size);
void kfree(void *address);

#endif //KERNEL_KALLOC_H
