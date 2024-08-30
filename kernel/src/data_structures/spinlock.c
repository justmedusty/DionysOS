//
// Created by dustyn on 6/21/24.
//

#include "include/data_structures/spinlock.h"


#include "include/arch//arch_atomic_operations.h"
#include <include/arch/arch_cpu.h>


void initlock(struct spinlock* spinlock, char* name) {
    spinlock->name = name;
    spinlock->locked = 0;
    spinlock->cpu = 0;
}

void acquire_spinlock(struct spinlock* spinlock) {
    arch_atomic_swap(&spinlock->locked, 1);
    spinlock->cpu = 0; //&cpu_list[mycpu()];
}

void release_spinlock(struct spinlock* spinlock) {
    spinlock->cpu = 0;
    spinlock->locked = 0;
}
