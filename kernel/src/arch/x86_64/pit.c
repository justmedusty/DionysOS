//
// Created by dustyn on 6/21/24.
//

#include "include/arch//arch_asm_functions.h"
#include "include/idt.h"
#include "include/pit.h"


uint64 pit_ticks = 0;


void pit_init() {
    outb(0x43, 0x36);
    uint16 div = (uint16)(1193180 / PIT_FREQ);
    outb(0x40, (uint8)div);
    outb(0x40, (uint8)(div >> 8));
    //load_idtr()
}

void pit_sleep(uint64 ms) {
    uint64 start = pit_ticks;
    while (pit_ticks - start < ms) {
        asm("nop");
    }
}