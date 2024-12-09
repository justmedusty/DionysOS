//
// Created by dustyn on 6/21/24.
//

#ifndef ARCH_TIMER_H
#define ARCH_TIMER_H
uint16_t timer_get_current_count();
void timer_set_frequency_hz(uint64_t freq);
void timer_init();
void timer_set_reload_value(uint16_t value);
void timer_sleep(uint16_t millis);
#endif