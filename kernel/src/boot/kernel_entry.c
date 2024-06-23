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



void kernel_bootstrap(){
    init_serial();
    gdt_init();
    idt_init();
    phys_init();
    heap_init();
    mem_bounds_init();
}