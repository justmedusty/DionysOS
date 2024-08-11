//
// Created by dustyn on 7/2/24.
//
#include "include/types.h"
#include "include/cpu.h"
#include "include/uart.h"
#include "include/arch/arch_lapic.h"

int8 panicked = 0;

void arch_panic(const char *str){
    write_string_serial("\nPanic! ");
    write_string_serial(str);
    panicked = 1;
    asm("cli");
    asm("hlt");
}

uint64 arch_mycpu(){
    return lapic_get_id();
}