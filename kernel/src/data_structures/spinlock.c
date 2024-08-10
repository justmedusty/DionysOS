//
// Created by dustyn on 6/21/24.
//

#include "spinlock.h"

#include <include/arch_asm_functions.h>

#include "include/arch_atomic_operations.h"


void initlock(struct spinlock *spinlock,char *name){
    spinlock->name = name;
    spinlock->locked = 0;
    spinlock->cpu = 0;
}

void acquire_spinlock(struct spinlock *spinlock){
    cli();
    arch_atomic_swap(&spinlock->locked,1);
    //spinlock->cpu = mycpu();
}