//
// Created by dustyn on 6/21/24.
//
#include "include/types.h"
#include "include/uart.h"
#include "include/arch/arch_local_interrupt_controller.h"
#include <include/arch/arch_paging.h>
#include <include/arch/arch_smp.h>
#include <include/arch/x86_64/msr.h>
#include <include/arch/x86_64/asm_functions.h>

uint64 apic_ticks = 0;
uint64 volatile lapic_base = 0;

void lapic_init() {
    serial_printf("Lapic init bit is %x.32\n",lapic_read(LAPIC_SPURIOUS));
    lapic_write((LAPIC_SPURIOUS), lapic_read(LAPIC_SPURIOUS) | 1 << 8 /* APIC software enable/disable bit*/);
    serial_printf("Lapic init bit is %x.32\n",lapic_read(LAPIC_SPURIOUS));
    lapic_calibrate_timer();
    serial_printf("LAPIC Initialised.\n");
}

void lapic_write(volatile uint32 reg, uint32 val) {
    if (lapic_base == 0) {
        lapic_base = (uint64)P2V(rdmsr(0x1b) & 0xFFFFF000);
    }
    mem_out((uint64*)lapic_base + reg, val);
}

uint32 lapic_read(uint32 reg) {
    if (lapic_base == 0) {
        lapic_base = (uint64)P2V(rdmsr(0x1b) & 0xFFFFF000);
    }
    return mem_in(lapic_base + reg);
}

uint32 get_lapid_id() {
    return (lapic_read(0x20) >> 24) & 0xFF;
}

void lapic_timer_stop() {
    // We do this to avoid overlapping oneshots
    lapic_write(LAPIC_TIMER_INITCNT, 0);
    lapic_write(LAPIC_TIMER_LVT, LAPIC_TIMER_DISABLE);
}

void lapic_oneshot(uint8 vec, uint64 ms) {
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

void lapic_send_all_int(uint32 id, uint32 vec) {
    lapic_ipi(id, vec | LAPIC_ICRAIS);
}

void lapic_send_others_int(uint32 id, uint32 vec) {
    lapic_ipi(id, vec | LAPIC_ICRAES);
}

uint32 lapic_get_id() {
}
