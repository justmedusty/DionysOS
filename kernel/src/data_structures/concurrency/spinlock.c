//
// Created by dustyn on 6/21/24.
//

#include "include/data_structures/spinlock.h"
#include "include/architecture//arch_atomic_operations.h"
#include <include/architecture/arch_cpu.h>
#include "include/architecture/arch_asm_functions.h"

bool bsp = true;

//TODO replace cli/sti with architecture agnostic wrapper function
void initlock(struct spinlock* spinlock, uint64_t id) {
    spinlock->id = id;
    spinlock->locked = 0;
    spinlock->cpu = 0;
}

void acquire_spinlock(struct spinlock* spinlock) {
    arch_atomic_swap(&spinlock->locked, 1);
    disable_interrupts();
    if (bsp == true) {
        return; // don't acquire any spinlocks during bootstrap because my_cpu won't work until the cpu setup is complete
    }
    spinlock->cpu = my_cpu();
}

void release_spinlock(struct spinlock* spinlock) {
    spinlock->cpu = NULL;
    enable_interrupts();
    spinlock->locked = 0;
}


