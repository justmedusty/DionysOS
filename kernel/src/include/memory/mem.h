//
// Created by dustyn on 6/18/24.
//

#ifndef KERNEL_MEM_H
#define KERNEL_MEM_H
#include "stddef.h"
#include "include/definitions/string.h"
void *memset(void *dest, int ch, size_t count);
void *memcpy(void *dest, const void *src, size_t count);
void *memmove(void *dest, const void *src, size_t count);
int memcmp(const void *lhs, const void *rhs, size_t count);
#endif //KERNEL_MEM_H
