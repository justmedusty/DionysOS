//
// Created by dustyn on 8/12/24.
//

#ifndef PIT_H
#define PIT_H
#include <stdint.h>
#include "stdbool.h"
#define PIT_FREQ 1193182
#define CHANNEL0_DATA 0x40
#define CHANNEL1_DATA 0x41
#define CHANNEL2_DATA 0x42
#define CMD 0x43

extern bool use_pit;
extern volatile uint64_t timer_ticks;
void x86_timer_interrupt();
void pit_init();
void pit_sleep(uint64_t ms);
void pit_set_reload_value(uint16_t new_reload_value);
void pit_set_freq(uint64_t freq);
uint64_t get_pit_ticks();
uint16_t pit_get_current_count();
#endif //PIT_H
