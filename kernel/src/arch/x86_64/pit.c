//
// Created by dustyn on 6/21/24.
//

#include "include/arch//arch_asm_functions.h"
#include "idt.h"
#include "pit.h"

#include <include/cpu.h>

#include "include/uart.h"
#include "include/arch/arch_smp.h"

uint64 pit_ticks = 0;

void pit_interrupt() {
    pit_ticks++;
    if(pit_ticks % 1000 == 0) {
            panic("1000 ticks");
    }
}

void pit_init() {
    outb(CMD, 0x36);
    uint16 div = (uint16)(1193180 / PIT_FREQ);
    outb(CHANNEL0_DATA, (uint8)div);
    outb(CHANNEL0_DATA, (uint8)(div >> 8));
    irq_register(bootstrap_lapic_id,pit_interrupt);
}

void pit_sleep(uint64 ms) {
    uint64 start = pit_ticks;
    while (pit_ticks - start < ms) {
        asm("nop");
    }
}