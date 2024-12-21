//
// Created by dustyn on 6/17/24.
//
#include <include/arch/x86_64/asm_functions.h>
#include <include/arch/x86_64/idt.h>
#include <include/scheduling/kthread.h>

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
#include "include/scheduling/sched.h"
#include "include/filesystem/diosfs.h"

/*
 *  BSP boostrapping.
 */
int32_t ready = 0;
void kernel_bootstrap() {
    init_serial();
    arch_init_segments();
    arch_setup_interrupts();
    arch_paging_init();
    phys_init();
    heap_init();
    mem_bounds_init();
    arch_vmm_init();
    acpi_init();
    lapic_init();
    vfs_init();
    diosfs_init(0);
    sched_init();
    smp_init();
    timer_init();
    serial_printf("Total Pages Allocated %i out of %i\n",total_allocated,usable_pages);
    kthread_init();
    ready = 1;
    scheduler_main();
}
