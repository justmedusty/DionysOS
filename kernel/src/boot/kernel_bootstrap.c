//
// Created by dustyn on 6/17/24.
//
#include "include/types.h"
#include "include/gdt.h"
#include "include/uart.h"
#include "include/idt.h"
#include "include/pmm.h"
#include "include/kheap.h"
#include "include/mem_bounds.h"
#include "include/arch_paging.h"
#include "include/arch_smp.h"

void kernel_bootstrap(){
    init_serial();
    arch_smp_query();
    arch_paging_init();
    gdt_init();
    idt_init();
    phys_init();
    heap_init();
    mem_bounds_init();
}