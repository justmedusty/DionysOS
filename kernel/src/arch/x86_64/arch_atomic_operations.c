//
// Created by dustyn on 7/2/24.
//
#include "include/types.h"
#include "include/arch//arch_atomic_operations.h"
#include "include/arch/x86_64/arch_asm_functions.h"

void arch_atomic_swap(uint64 *field, uint64 new_value){
    // The xchg is atomic.
    while(xchg(field, new_value) != 0)
        ;

    // Tell the C compiler and the processor to not move loads or stores
    // past this point, to ensure that the critical section's memory
    // references happen after the lock is acquired.
    __sync_synchronize();
    return;
}