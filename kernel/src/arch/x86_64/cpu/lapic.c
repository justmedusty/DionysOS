//
// Created by dustyn on 6/21/24.
//
#include "include/types.h"
#include "include/uart.h"
#include "include/arch/arch_local_interrupt_controller.h"
#include <include/arch/arch_paging.h>

uint64 apic_ticks = 0;

void lapic_init() {
    lapic_write(0xf0, 0x1ff);
    serial_printf("LAPIC Initialised.\n");
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
    lapic_timer_stop();
}

void lapic_write(uint32 reg, uint32 val) {
    *((volatile uint32*)(P2V(0xfee00000) + reg)) = val;
}

uint32 lapic_read(uint32 reg) {
    return *((volatile uint32*)(P2V(0xfee00000) + reg));
}

void lapic_eoi() {
    lapic_write((uint8)0xb0, 0x0);
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
    return lapic_read(LAPIC_ID_REG) >> LAPIC_ICDESTSHIFT;
}