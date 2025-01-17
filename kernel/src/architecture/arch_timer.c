//
// Created by dustyn on 12/9/24.
//

#ifdef __x86_64__

#include "include/architecture/x86_64/pit.h"
#include "stdint.h"
#include "include/architecture/x86_64/hpet.h"

/*
 *  We can use function pointers instead of branches but this is okay for now.
 */
uint16_t timer_get_current_count() {
    return timer_ticks;
}

void timer_set_frequency_hz(uint64_t freq) {
    if (use_pit) {
        pit_set_freq(freq);
        return;
    } else {

    }

}

void timer_init(uint64_t hz) {
    if (use_pit) {
        pit_init();
    } else {
        hpet_initialize_and_enable_interrupts(hz);
    }
}

void timer_set_reload_value(uint16_t value) {
    if (use_pit) {
        pit_set_reload_value(value);
    } else {

    }

}

void timer_sleep(uint16_t millis) {
    pit_sleep(millis);
}

#endif