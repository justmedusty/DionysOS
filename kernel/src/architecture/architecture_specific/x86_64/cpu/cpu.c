//
// Created by dustyn on 7/2/24.
//
#include <include/architecture/x86_64/gdt.h>
#include <include/architecture/x86_64/idt.h>
#include <include/architecture/arch_cpu.h>
#include <include/architecture/arch_interrupts.h>
#include <include/architecture/arch_memory_init.h>
#include <include/architecture/arch_smp.h>
#include <include/architecture/arch_timer.h>
#include <include/architecture/arch_vmm.h>
#include <include/architecture/x86_64/asm_functions.h>
#include <include/data_structures/spinlock.h>
#include <include/definitions/string.h>
#include <include/memory/kalloc.h>
#include <include/scheduling/sched.h>
#include <include/scheduling/kthread.h>

#include "include/types.h"
#include "include/drivers/serial/uart.h"
#include "include/architecture/arch_local_interrupt_controller.h"
#include "limine.h"
extern volatile int32_t ready;
uint8_t panicked = 0;
struct cpu cpu_list[16];
struct queue local_run_queues[16];

struct spinlock bootstrap_lock;
#ifdef __x86_64__

void panic(const char* str) {
    cli();
    serial_printf("\nPanic! %s ",str);
    panicked = 1; /* The next timer interrupt other CPUs will see this and also halt*/
    for (;;) {
        asm("hlt");
        asm("nop");
    }
}

struct cpu* my_cpu() {
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
    reload_vmm();
    lapic_init();
    serial_printf("CPU %x.8  online, LAPIC ID %x.8 \n",smp_info->processor_id,get_lapid_id());

    if(get_lapid_id() == 0) {
        panic("CANNOT GET LAPIC ID\n");
    }

    if (my_cpu()->scheduler_state == NULL) {
     my_cpu()->scheduler_state = kmalloc(sizeof(struct gpr_state));
    }
    my_cpu()->page_map = kernel_pg_map;

    release_spinlock(&bootstrap_lock);
    cpus_online++;
    while (!ready){} /* Just to make entry print message cleaner and grouped together */
    kthread_init();
    scheduler_main();
}
#endif