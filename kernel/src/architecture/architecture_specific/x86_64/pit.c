//
// Created by dustyn on 6/21/24.
//
#pragma once
#include <include/architecture/arch_cpu.h>
#include "include/architecture/generic_asm_functions.h"
#include "include/architecture/x86_64/idt.h"
#include "include/architecture/x86_64/pit.h"
#include "include/drivers/serial/uart.h"
#include "include/architecture/arch_local_interrupt_controller.h"

static uint64_t pit_ticks = 0;

/*
 * The timer intterupt for the x86 PIT timer.
 * Only the BSP will be interrupted, after which it will invoke a broadcast interrupt after incrementing the tick counter.
 * After the broadcast, every CPU will be interrupted and do a panic check and then ack the interrupt.
 * Later this will also do scheduler preemption and the like.
 */

void pit_interrupt() {
    if(my_cpu()->lapic_id == 0) {
        pit_ticks++;
        lapic_broadcast_interrupt(32 + 0 /* Broadcast IPI to all other processes so they can do their own preemption checks or panic checks */);
    }

    if(panicked) {
        for(;;) {
            cli();
            asm volatile("hlt");
        }
    }

    //Do preemption stuff, only count ticks on processor 0
    lapic_eoi();
}
/*
 * Get the current internal PIT ticks, you need to read the low and high byte and concat to a short and return
 */
uint16_t pit_get_current_count() {
    write_port(CMD,0);
    uint8_t low = read_port(CHANNEL0_DATA);
    uint8_t high = read_port(CHANNEL0_DATA);
    return (uint16_t) high << 8 | low;
}

/*
 *  Get current pit ticks
 */
uint64_t get_pit_ticks() {
    return pit_ticks;
}

/*
 * Set the PIT frequency (in HZ)
 */
void pit_set_freq(uint64_t freq) {
  uint64_t new_divisor = PIT_FREQ / freq;

  if(PIT_FREQ % freq > freq / 2){
      new_divisor++;
      }
	pit_set_reload_value(new_divisor);
}

/*
 * Helper function for pit_set_freq,
 * helps set a new timer frequency
 */
void pit_set_reload_value(uint16_t new_reload_value) {
 	write_port(CMD,0x34);
 	write_port(CHANNEL0_DATA,(uint8_t)new_reload_value & 0xFF);
 	write_port(CHANNEL0_DATA,(uint8_t)new_reload_value >> 8);
}
/*
 * Init the PIT timer to 18HZ, and register it at IRQ 0, passing a pointer to the pit_intterupt function to be called on interrupt
 */
void pit_init() {
    pit_set_freq(18);
    irq_register(0,pit_interrupt);
    serial_printf("Timer inititialized\n");
}

/*
 * Sleep until x Pit ticks have passed
 */
void pit_sleep(uint64_t ms) {
    uint64_t start = pit_ticks;
    while ((pit_ticks - start) < ms) {
        asm volatile("nop");
    }
}