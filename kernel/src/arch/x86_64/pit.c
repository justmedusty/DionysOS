//
// Created by dustyn on 6/21/24.
//
#pragma once
#include <include/arch/arch_cpu.h>
#include "include/arch/x86_64/asm_functions.h"
#include "idt.h"
#include "pit.h"
#include <include/arch/arch_cpu.h>
#include "include/uart.h"
#include "include/arch/arch_smp.h"
#include "include/arch/arch_local_interrupt_controller.h"

uint64 pit_ticks = 0;

void pit_interrupt() {

    if(arch_mycpu()->lapic_id == 0) {
        pit_ticks++;
        lapic_broadcast_interrupt(32 + 0 /* Broadcast IPI to all other processes so they can do their own preemption checks or panic checks */);
    }

    if(panicked) {
        for(;;) {
            cli();
            asm volatile("hlt");
        }
    }

    serial_printf("PIT interrupt at CPU %x.8  \n",arch_mycpu()->lapic_id);
    //Do preemption stuff, only count ticks on processor 0
    lapic_eoi();
}

uint16 pit_get_current_count() {
    outb(CMD,0);
    uint8 low = inb(CHANNEL0_DATA);
    uint8 high = inb(CHANNEL0_DATA);
    return (uint16) high << 8 | low;
}

uint64 get_pit_ticks() {
    return pit_ticks;
}

void pit_set_freq(uint64 freq) {
  uint64 new_divisor = PIT_FREQ / freq;

  if(PIT_FREQ % freq > freq / 2){
      new_divisor++;
      }
	pit_set_reload_value(new_divisor);
}

void pit_set_reload_value(uint16 new_reload_value) {
 	outb(CMD,0x34);
 	outb(CHANNEL0_DATA,(uint8)new_reload_value & 0xFF);
 	outb(CHANNEL0_DATA,(uint8)new_reload_value >> 8);
}

void pit_init() {
    pit_set_freq(18);
    irq_register(0,pit_interrupt);
    serial_printf("Timer inititialized\n");
    asm volatile("int $0x20");
}


void pit_sleep(uint64 ms) {
    uint64 start = pit_ticks;
    while (pit_ticks - start < ms) {
        asm("nop");
    }
}