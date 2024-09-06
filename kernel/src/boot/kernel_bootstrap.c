//
// Created by dustyn on 6/17/24.
//
#include <arch/x86_64/idt.h>

#include "include/types.h"
#include "include/arch/arch_memory_init.h"
#include "include/uart.h"
#include "include/arch/arch_interrupts.h"
#include "include/pmm.h"
#include "include/kalloc.h"
#include "include/mem_bounds.h"
#include "include/arch/arch_paging.h"
#include "include/arch/arch_smp.h"
#include "include/vmm.h"
#include "include/slab.h"
#include <include/arch/arch_cpu.h>
#include "include/acpi.h"
#include "include/madt.h"
#include "include/arch/arch_global_interrupt_controller.h"
#include "include/arch/arch_timer.h"
#include "include/arch/arch_local_interrupt_controller.h"

/*
 *  BSP Processor boostrapping.
 */
void kernel_bootstrap(){
    init_serial();
    arch_init_segments();
    irq_handler_init();
    arch_setup_interrupts();
    arch_paging_init();
    phys_init();
    heap_init();
    mem_bounds_init();
    vmm_init();
    acpi_init();
    lapic_init();
    smp_init();
    arch_timer_init();
}