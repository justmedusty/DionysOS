//
// Created by dustyn on 7/2/24.
//
#include "include/types.h"
#include "include/cpu.h"
#include "include/uart.h"

int8 panicked = 0;

void panic(const char *str){
    write_string_serial("\nPanic!\n");
    write_string_serial(str);
    panicked = 1;
    asm("cli");
    asm("hlt");
}