//
// Created by dustyn on 7/2/24.
//
#include "include/types.h"
#include "include/cpu.h"
#include "include/uart.h"

int8 panicked = 0;

void panic(const char *str){
    serial_printf("%s\nPanic!\n",str);
    panicked = 1;
    asm("cli");
    asm("hlt");
}