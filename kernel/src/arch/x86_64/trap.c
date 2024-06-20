//
// Created by dustyn on 6/19/24.
//
#include "include/types.h"
#include "include/trapframe.h"
#include "include/uart.h"
void trap(uint8 trap_no){
    write_string_serial("TRAP\n");
    write_int_serial(trap_no);
    switch (trap_no) {
        default:
            write_string_serial("TRAP\n");

    }

}