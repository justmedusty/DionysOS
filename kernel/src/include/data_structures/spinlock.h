//
// Created by dustyn on 6/21/24.
//

#pragma once
#include "include/types.h"
#include <include/arch/arch_cpu.h>
// Mutual exclusion lock.
//Making this 256 bits, trying to force power of 2 alignment will make things easier later
struct spinlock{
    uint64 locked;
    uint64 id;
    cpu *cpu;
    uint64 program_counters[10];
};


void initlock(struct spinlock *spinlock,uint64 id);
void acquire_spinlock(struct spinlock *spinlock);
void release_spinlock(struct spinlock *spinlock);