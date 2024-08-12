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
#endif //PIT_H
