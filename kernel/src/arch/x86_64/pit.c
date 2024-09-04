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
    pit_ticks++;
    lapic_eoi();
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
 	outb(CHANNEL0_DATA,(uint8)new_reload_value);
 	outb(CHANNEL0_DATA,(uint8)new_reload_value >> 8);
}

void pit_init() {
    pit_set_freq(10);
    irq_register(0,pit_interrupt);
    serial_printf("Timer inititialized Ticks : %x.64\n",pit_ticks);
}

void pit_sleep(uint64 ms) {
    uint64 start = pit_ticks;
    while (pit_ticks - start < ms) {
        asm("nop");
    }
}