//
// Created by dustyn on 6/17/24.
//
#include <include/arch/x86_64/idt.h>
#include "include/filesystem/vfs.h"
#include "include/arch/arch_memory_init.h"
#include "include/drivers/serial/uart.h"
#include "include/arch/arch_interrupts.h"
#include "include/mem/pmm.h"
#include "include/mem/mem_bounds.h"
#include "include/arch/arch_paging.h"
#include "include/arch/arch_smp.h"
#include "include/mem/vmm.h"
#include "include/mem/slab.h"
#include "include/arch/x86_64/acpi.h"
#include "include/arch/arch_timer.h"
#include "include/arch/arch_local_interrupt_controller.h"
#include "include/scheduling/dfs.h"
#include "include/filesystem/tempfs.h"

/*
 *  BSP boostrapping.
 */
void kernel_bootstrap() {
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
    vfs_init();
    tempfs_init();
    smp_init();
    arch_timer_init();
    dfs_init();

    for (;;) {
        asm volatile("nop");
    }
}
