//
// Created by dustyn on 6/21/24.
//

#include "include/data_structures/spinlock.h"
#include "include/architecture//arch_atomic_operations.h"
#include <include/architecture/arch_cpu.h>
#include "include/architecture/arch_asm_functions.h"

bool bsp = true;

void initlock(struct spinlock *spinlock, uint64_t id) {
    spinlock->id = id;
    spinlock->locked = 0;
    spinlock->cpu = 0;
}

void acquire_spinlock(struct spinlock *spinlock) {
    if (bsp == true) {
        /*
         * Spinlocks are acquired extensively during bootstrap for data structure functions and this cannot be changed
         * without making specialized bootstrap functions or something similar which is wasteful. It is much easier to ust
         * do a quick bool check on acquire
         */
        return;
        // don't bother with spinlocks during bootstrap because my_cpu won't work until the cpu setup is complete
    }
    arch_atomic_swap(&spinlock->locked, 1);
    disable_interrupts();

    spinlock->cpu = my_cpu();
}

void release_spinlock(struct spinlock *spinlock) {
    spinlock->cpu = NULL;
    enable_interrupts();
    spinlock->locked = 0;
}

bool try_lock(struct spinlock *spinlock) {
    if(!arch_atomic_swap_or_return(&spinlock->locked,1)){
        return false;
    }
    disable_interrupts();

    spinlock->cpu = my_cpu();
    return true;
}
