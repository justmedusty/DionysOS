//
// Created by dustyn on 6/21/24.
//

#include "include/data_structures/spinlock.h"
#include "include/architecture//arch_atomic_operations.h"
#include <include/architecture/arch_cpu.h>
#include "include/architecture/arch_asm_functions.h"

//TODO replace cli/sti with architecture agnostic wrapper function
void initlock(struct spinlock* spinlock, uint64_t id) {
    spinlock->id = id;
    spinlock->locked = 0;
    spinlock->cpu = 0;
}

void acquire_spinlock(struct spinlock* spinlock) {
    arch_atomic_swap(&spinlock->locked, 1);
    spinlock->cpu = 0;
}

void release_spinlock(struct spinlock* spinlock) {
    spinlock->cpu = 0;
    spinlock->locked = 0;
}

void acquire_interrupt_safe_spinlock(struct spinlock* spinlock) {
    arch_atomic_swap(&spinlock->locked, 1);
    disable_interrupts();
    spinlock->cpu = 0;
}

void release_interrupt_safe_spinlock(struct spinlock* spinlock) {
    spinlock->cpu = 0;
    spinlock->locked = 0;
    enable_interrupts();
}
