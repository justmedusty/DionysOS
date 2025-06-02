//
// Created by dustyn on 6/21/24.
//

#pragma once
#include "include/definitions/types.h"
#include <include/architecture/arch_cpu.h>
// Mutual exclusion lock.
//Making this 256 bytes, trying to force power of 2 alignment will make things easier later
struct spinlock{
    uint64_t locked;
    uint64_t id;
    struct cpu *cpu;
    void *reserved;
};

//bootstrap bool so we can avoid cpu stuff while boostrapping
extern bool bsp;

void initlock(struct spinlock *spinlock,uint64_t id);
void acquire_spinlock(struct spinlock *spinlock);
void release_spinlock(struct spinlock *spinlock);
bool try_lock(struct spinlock *spinlock);
