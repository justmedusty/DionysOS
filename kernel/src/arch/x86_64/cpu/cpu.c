//
// Created by dustyn on 7/2/24.
//
#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include <include/arch/arch_cpu.h>
#include <include/arch/arch_interrupts.h>
#include <include/arch/arch_memory_init.h>
#include <include/arch/arch_smp.h>
#include <include/arch/arch_timer.h>
#include <include/arch/arch_vmm.h>
#include <include/arch/x86_64/asm_functions.h>
#include <include/data_structures/spinlock.h>

#include "include/types.h"
#include "include/drivers/uart.h"
#include "include/arch/arch_local_interrupt_controller.h"
#include "limine.h"

uint8 panicked = 0;
cpu cpu_list[16];
struct queue local_run_queues[16];

struct spinlock bootstrap_lock;
#ifdef __x86_64__

void panic(const char* str) {
    cli();
    serial_printf("\nPanic! %s ",str);
    panicked = 1;
    for (;;) {
        asm("hlt");
        asm("nop");
    }
}

cpu* my_cpu() {
    return &cpu_list[get_lapid_id()];
}

struct process* current_process() {
    return cpu_list[get_lapid_id()].running_process;
}

void arch_initialise_cpu( struct limine_smp_info *smp_info) {
    acquire_spinlock(&bootstrap_lock);
    serial_printf("\nInitialising CPU... \n\n");
    gdt_reload();
    idt_reload();
    arch_reload_vmm();
    lapic_init();
    serial_printf("CPU %x.8  online, LAPIC ID %x.8 \n",smp_info->processor_id,get_lapid_id());
    if(get_lapid_id() == 0) {
        panic("CANNOT GET LAPIC ID\n");
    }
    my_cpu()->page_map = kernel_pg_map;
    release_spinlock(&bootstrap_lock);
    cpus_online++;
    for(;;) {
        asm volatile("nop");
    }

}
#endif
