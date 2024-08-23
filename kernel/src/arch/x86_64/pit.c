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
    serial_printf("pit tick\n");
}

void pit_set_freq(uint64 freq) {
  uint64 new_divisor = PIT_FREQ / freq;
  if(PIT_FREQ % freq > freq / 2){
      new_divisor++;
      }

}

void pit_set_reload_value(uint16 new_reload_value) {
 	outb(CMD,0x34);
 	outb(CHANNEL0_DATA,(uint8)new_reload_value);
 	outb(CHANNEL0_DATA,(uint8)new_reload_value >> 8);
}

void pit_init() {
    outb(CMD, 0x36);
    uint16 div = (uint16)(1193180 / PIT_FREQ);
    outb(CHANNEL0_DATA, (uint8)div);
    outb(CHANNEL0_DATA, (uint8)(div >> 8));
    irq_register(0,&pit_interrupt);
}

void pit_sleep(uint64 ms) {
    uint64 start = pit_ticks;
    while (pit_ticks - start < ms) {
        asm("nop");
    }
}