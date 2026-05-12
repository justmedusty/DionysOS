//
// Created by dustyn on 6/21/24.
//

#include "include/data_structures/spinlock.h"
#include "include/architecture//arch_atomic_operations.h"
#include <include/architecture/arch_cpu.h>
#include "include/architecture/arch_asm_functions.h"

bool bsp = true;

void initlock(struct spinlock *spinlock, uint64_t id) {
    if ((uint64_t )spinlock ==  0xFFFF8001C8C00018 || (uint64_t )spinlock ==  0xFFFF8001C8C60E80 ) {
        err_printf("INIT ~ LOCK ADDR %x.64 HOLDING PROC %x.64 CPU %x.64 RECURSION DEPTH %i ID %i LOCKED %i\n",spinlock,spinlock->holding_process,spinlock->cpu,spinlock->recursion_depth,spinlock->id,spinlock->locked);
        serial_printf("INIT ~ LOCK ADDR %x.64 HOLDING PROC %x.64 CPU %x.64 RECURSION DEPTH %i ID %i LOCKED %i\n",spinlock,spinlock->holding_process,spinlock->cpu,spinlock->recursion_depth,spinlock->id,spinlock->locked);
    }
    spinlock->id = id;
    spinlock->locked = 0;
    spinlock->cpu = 0;
    spinlock->recursion_depth = 0;
}
//TODO check int flag instead of just turning them back on
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

  if ((uint64_t )spinlock ==  0xFFFF8001C8C00018 || (uint64_t )spinlock ==  0xFFFF8001C8C60E80 ) {
        serial_printf("BAD SPINLOCK ACQUIRED\n");
    }
    //Allow recursive locking
    if(spinlock->cpu == my_cpu() && spinlock->holding_process == current_process()){
        spinlock->recursion_depth++;
        return;
    }

    disable_interrupts();
    bool ret = arch_atomic_swap(&spinlock->locked, 1);
    if (!ret) {

        err_printf("LOCK ADDR %x.64 HOLDING PROC %x.64 CPU %x.64 RECURSION DEPTH %i ID %i LOCKED %i\n",spinlock,spinlock->holding_process,spinlock->cpu,spinlock->recursion_depth,spinlock->id,spinlock->locked);
        serial_printf("LOCK ADDR %x.64 HOLDING PROC %x.64 CPU %x.64 RECURSION DEPTH %i ID %i LOCKED %i\n",spinlock,spinlock->holding_process,spinlock->cpu,spinlock->recursion_depth,spinlock->id,spinlock->locked);
        if (spinlock->holding_process != NULL && current_process() != NULL && my_cpu() != NULL){
            err_printf("Deadlock found , holding process is %i , holding cpu is %i\nNew process is %i, new cpu is %i\n",spinlock->holding_process->process_id,spinlock->cpu,current_process()->process_id,my_cpu()->cpu_id);
            serial_printf("Deadlock found , holding process is %i , holding cpu is %i\nNew process is %i, new cpu is %i\n",spinlock->holding_process->process_id,spinlock->cpu,current_process()->process_id,my_cpu()->cpu_id);
        }

    }
    spinlock->cpu = my_cpu();
    spinlock->holding_process = current_process();

}

void release_spinlock(struct spinlock *spinlock) {
    if ((uint64_t )spinlock ==  0xFFFF8001C8C00018 || (uint64_t )spinlock ==  0xFFFF8001C8C60E80 ) {
        serial_printf("BAD SPINLOCK RELEASED\n");
    }
    if (spinlock->recursion_depth > 0) {
        spinlock->recursion_depth--;
    }

    if (spinlock->recursion_depth == 0) {
        spinlock->cpu = NULL;
        spinlock->holding_process = NULL;
        spinlock->locked = 0;
        spinlock->recursion_depth = 0;
        enable_interrupts();
    }
}

bool try_lock(struct spinlock *spinlock) {

    if(!arch_atomic_swap_or_return(&spinlock->locked,1)){
        return false;
    }
    disable_interrupts();
    if(spinlock->cpu == my_cpu()){
        spinlock->recursion_depth++;
    }
    spinlock->cpu = my_cpu();
    return true;
}
