//
// Created by dustyn on 8/11/24.
//

#include "../include/arch/arch_memory_init.h"
#include "../include/arch/x86_64/gdt.h"
void arch_init_segments(){
    gdt_init();
}