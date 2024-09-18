//
// Created by dustyn on 6/21/24.
//
#include <arch/x86_64/gdt.h>
#include <include/arch/arch_cpu.h>
#include <include/arch/arch_global_interrupt_controller.h>
#include "include/types.h"
#include "include/drivers/uart.h"
#include "include/arch/arch_local_interrupt_controller.h"
#include <include/arch/arch_paging.h>
#include <include/arch/arch_smp.h>
#include <include/arch/x86_64/msr.h>
#include <include/arch/x86_64/asm_functions.h>

uint64 apic_ticks = 0;
uint64 lapic_base = 0;

void lapic_init() {
    pic_disable();
    //Enable lapic by writing to the enable/disable bit to the left of the spurious vector
    lapic_write((LAPIC_SPURIOUS), lapic_read(LAPIC_SPURIOUS) | 1 << 8 /* APIC software enable/disable bit*/);
    //enable LAPIC via the IA32 MSR enable bit
    lapic_calibrate_timer();
    serial_printf("LAPIC Initialised.\n");

    //Assign the TSS of the bootstrap processor since we can't do it in the same spot we do the others.
    if(mycpu()->lapic_id == 0) {
        mycpu()->tss = &tss[0];
    }
}


void lapic_write(uint32 reg, uint32 val) {
    if (lapic_base == 0) {
        lapic_base = rdmsr(IA32_APIC_BASE_MSR) & 0xFFFFF000;
    }
    *((volatile uint32*)(P2V(lapic_base) + reg)) = val;
}

uint32 lapic_read(uint32 reg) {
    if (lapic_base == 0) {
        lapic_base = rdmsr(IA32_APIC_BASE_MSR) & 0xFFFFF000;
    }
    return *((volatile uint32*)(P2V(lapic_base) + reg));
}

uint32 get_lapid_id() {
    return lapic_read(LAPIC_ID_REG) >> 24;
}

void lapic_timer_stop() {
    // We do this to avoid overlapping oneshots
    lapic_write(LAPIC_TIMER_INITCNT, 0);
    lapic_write(LAPIC_TIMER_LVT, LAPIC_TIMER_DISABLE);
}

void lapic_timer_oneshot(uint8 vec, uint64 ms) {
    lapic_timer_stop();
    lapic_write(LAPIC_TIMER_DIV, 0);
    lapic_write(LAPIC_TIMER_LVT, vec);
    lapic_write(LAPIC_TIMER_INITCNT, apic_ticks * ms);
}

void lapic_calibrate_timer() {
    lapic_timer_stop();
    lapic_write(LAPIC_TIMER_DIV, 0);
    lapic_write(LAPIC_TIMER_LVT, (1 << 16) | 0xff);
    lapic_write(LAPIC_TIMER_INITCNT, 0xFFFFFFFF);
    lapic_write(LAPIC_TIMER_LVT, LAPIC_TIMER_DISABLE);
    uint32 ticks = 0xFFFFFFFF - lapic_read(LAPIC_TIMER_CURCNT);
    apic_ticks = ticks;
    serial_printf("LAPIC ticks %x.64\n", apic_ticks);
    lapic_timer_stop();
}

void lapic_eoi() {
    lapic_write(LAPIC_EOI, 0x0);
}

void lapic_ipi(uint32 id, uint8 dat) {
    lapic_write(LAPIC_ICRHI, id << LAPIC_ICDESTSHIFT);
    lapic_write(LAPIC_ICRLO, dat);
}

void lapic_broadcast_interrupt(uint32 vec) {

    for (int i = 0; i < cpu_count; i++) {
        if (i == mycpu()->lapic_id) {
            continue;
        }

        if (cpu_list[i].lapic_id == i) {
            lapic_send_int(i,vec);
        }
    }
}


void lapic_send_int(uint32 id, uint32 vec) {
    lapic_ipi(id, vec | LAPIC_ICRAES);
}
