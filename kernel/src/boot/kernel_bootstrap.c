//
// Created by dustyn on 6/17/24.
//
#include <include/architecture/x86_64/asm_functions.h>
#include <include/architecture/x86_64/idt.h>
#include <include/device/display/framebuffer.h>
#include <include/scheduling/kthread.h>
#include "include/filesystem/vfs.h"
#include "include/architecture/arch_memory_init.h"
#include "include/drivers/serial/uart.h"
#include "include/architecture/arch_interrupts.h"
#include "include/memory/pmm.h"
#include "include/memory/mem_bounds.h"
#include "include/architecture/arch_paging.h"
#include "include/architecture/arch_smp.h"
#include "include/memory/vmm.h"
#include "include/memory/slab.h"
#include "include/architecture/x86_64/acpi.h"
#include "include/architecture/arch_timer.h"
#include "include/architecture/arch_local_interrupt_controller.h"
#include "include/scheduling/sched.h"
#include "include/filesystem/diosfs.h"

/*
 *  BSP boostrapping.
 */
int32_t ready = 0;

void welcome_message() {
    kprintf_color(RED, DIONYSOS_ASCII_STRING);
    kprintf_color(RED,AUTHOR_ASCII_STRING);
    kprintf_color(
        WHITE,
        "Welcome to the DionysOS Operating System, written by Dustyn Gibb. If you wish to contribute you are free to open PRs however you should speak with me first.\n");
    kprintf_color(CYAN, "DionysOS Booting...\n");
}


void kernel_bootstrap() {
    initlock(framebuffer_device.lock, FRAME_LOCK);
    welcome_message();
    init_serial();
    arch_init_segments();
    arch_setup_interrupts();
    arch_paging_init();
    phys_init();
    heap_init();
    mem_bounds_init();
    arch_vmm_init();
    init_system_device_tree();
    insert_device_into_kernel_tree(&serial_device); // this is done late because memory is not setup yet until here and will just crash the system if we try to do this any earlier
    framebuffer_init();
    acpi_init();
    lapic_init();
    vfs_init();
    diosfs_init(0);
    bsp = false;
    // set bsp bool for acquire_spinlock so that my_cpu will be called and assigned when a processor takes a lock
    smp_init();
    timer_init();
    info_printf( "Total MB Allocated %i out of %i\n", (total_allocated * (PAGE_SIZE / 1024)) / 1024,
                  (usable_pages * (PAGE_SIZE / 1024)) / 1024);
    kprintf_color(CYAN, "Kernel Boot Complete\n");
    kthread_init();
    ready = 1;
    scheduler_main();
}
