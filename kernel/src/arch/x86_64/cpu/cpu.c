//
// Created by dustyn on 7/2/24.
//
#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include <include/arch/arch_cpu.h>
#include <include/arch/arch_interrupts.h>
#include <include/arch/arch_memory_init.h>
#include <include/arch/arch_vmm.h>
#include <include/arch/x86_64/asm_functions.h>
#include <include/data_structures/spinlock.h>

#include "include/types.h"
#include "include/uart.h"
#include "include/arch/arch_local_interrupt_controller.h"
#include "limine.h"

int8 panicked = 0;
cpu cpu_list[8];
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

uint64 arch_mycpu() {
}

void arch_initialise_cpu( struct limine_smp_info *smp_info) {
    acquire_spinlock(&bootstrap_lock);
    gdt_reload();
    idt_reload();
    arch_reload_vmm();
    lapic_init();
    serial_printf("CPU %x.8  online, LAPIC ID %x.32\n",smp_info->processor_id,get_lapid_id());
    release_spinlock(&bootstrap_lock);
    for(;;) {
        asm("hlt");
    }

}
#endif
