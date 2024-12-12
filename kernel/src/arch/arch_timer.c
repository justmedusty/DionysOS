//
// Created by dustyn on 12/9/24.
//

#ifdef __x86_64__
#include "include/arch/x86_64/pit.h"
#include "stdint.h"

uint16_t timer_get_current_count() {
    return get_pit_ticks();
}

void timer_set_frequency_hz(uint64_t freq) {
  pit_set_freq(freq);
}

void timer_init() {
  pit_init();
}

void timer_set_reload_value(uint16_t value) {
  pit_set_reload_value(value);
}

void timer_sleep(uint16_t millis) {
  pit_sleep(millis);
}

#endif