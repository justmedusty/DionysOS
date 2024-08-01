//
// Created by dustyn on 6/17/24.
//
#include "include/types.h"
#include "include/gdt.h"
#include "include/uart.h"
#include "include/idt.h"
#include "include/pmm.h"
#include "include/kalloc.h"
#include "include/mem_bounds.h"
#include "include/arch_paging.h"
#include "include/arch_smp.h"
#include "include/arch_vmm.h"
#include "include/slab.h"
#include "include/cpu.h"

void kernel_bootstrap(){
    init_serial();
    gdt_init();
    idt_init();
    arch_smp_query();
    arch_paging_init();
    phys_init();
    heap_init();
    mem_bounds_init();
    arch_init_vmm();
}