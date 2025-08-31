//
// Created by dustyn on 7/2/24.
//
#include "include/definitions/types.h"
#include "include/architecture//arch_atomic_operations.h"
#include "include/architecture/x86_64/asm_functions.h"

void arch_atomic_swap(uint64_t *field, uint64_t new_value){
    // The xchg is atomic.
    while(xchg(field, new_value) != 0)
        ;

    // Tell the C compiler and the processor to not move loads or stores
    // past this point, to ensure that the critical section's memory
    // references happen after the lock is acquired.
    __sync_synchronize(); /* for x86 this is just an mfence instruction , you can also put an empty inline asm function and declare it as changing memory */
}

bool arch_atomic_swap_or_return(uint64_t *field, uint64_t new_value){
    // The xchg is atomic.
    if(xchg(field, new_value) != 0){
        return false;
    }

    // Tell the C compiler and the processor to not move loads or stores
    // past this point, to ensure that the critical section's memory
    // references to happen after the lock is acquired.
    __sync_synchronize(); /* for x86 this is just an mfence instruction , you can also put an empty inline asm function and declare it as changing memory */
    return true;
}