//
// Created by dustyn on 8/12/24.
//

#ifndef PIT_H
#define PIT_H
#define PIT_FREQ 1193182
#define CHANNEL0_DATA 0x40
#define CHANNEL1_DATA 0x41
#define CHANNEL2_DATA 0x42
#define CMD 0x43


void pit_interrupt();
void pit_init();
void pit_sleep(uint64 ms);
void pit_set_reload_value(uint16 new_reload_value);
void pit_set_freq(uint64 freq);
uint64 get_pit_ticks();
uint16 pit_get_current_count();
#endif //PIT_H
