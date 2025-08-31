//
// Created by dustyn on 8/11/24.
//

#include "include/architecture/arch_memory_init.h"
#include "include/architecture/x86_64/gdt.h"
#ifdef __x86_64__
void arch_init_segments(){
    gdt_init();
}
#endif