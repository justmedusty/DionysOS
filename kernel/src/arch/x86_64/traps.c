//
// Created by dustyn on 6/19/24.
//
#include "include/types.h"
#include "include/trapframe.h"
#include "include/uart.h"

//Exception 0
void divide_by_zero(){
    write_string_serial("Divide By Zero Occurred\n");
    asm("hlt");
}
//Exception 1
void debug_exception(){
    write_string_serial("Debug Exception Occurred\n");
    asm("hlt");
}

// Exception 2: Non-Maskable Interrupt
void nmi_interrupt() {
    write_string_serial("Non-Maskable Interrupt Occurred\n");
    asm("hlt");
}

// Exception 3: Breakpoint
void breakpoint() {
    write_string_serial("Breakpoint Occurred\n");
}

// Exception 4: Overflow
void overflow() {
    write_string_serial("Overflow Occurred\n");
    asm("hlt");
}

// Exception 5: Bounds Check
void bounds_check() {
    write_string_serial("Bounds Check Exception Occurred\n");
}

// Exception 6: Illegal Opcode
void illegal_opcode() {
    write_string_serial("Illegal Opcode Exception Occurred\n");
    asm("hlt");
}

// Exception 7: Device Not Available
void device_not_available() {
    write_string_serial("Device Not Available Exception Occurred\n");
    asm("hlt");
}

// Exception 8: Double Fault
void double_fault() {
    write_string_serial("Double Fault Occurred\n");
    asm("hlt");
}

// Exception 10: Invalid Task Switch Segment
void invalid_tss() {
    write_string_serial("Invalid Task Switch Segment Occurred\n");
    asm("hlt");
}

// Exception 11: Segment Not Present
void segment_not_present() {
    write_string_serial("Segment Not Present Exception Occurred\n");
    asm("hlt");
}

// Exception 12: Stack Exception
void stack_exception() {
    write_string_serial("Stack Exception Occurred\n");
    asm("hlt");
}

// Exception 13: General Protection Fault
void general_protection_fault() {
    write_string_serial("General Protection Fault Occurred\n");
    asm("hlt");
}

// Exception 14: Page Fault
void page_fault() {
    write_string_serial("Page Fault Occurred\n");
    asm("hlt");
}

// Exception 16: Floating Point Error
void floating_point_error() {
    write_string_serial("Floating Point Error Occurred\n");
    asm("hlt");
}

// Exception 17: Alignment Check
void alignment_check() {
    write_string_serial("Alignment Check Occurred\n");
    asm("hlt");
}

// Exception 18: Machine Check
void machine_check() {
    write_string_serial("Machine Check Occurred\n");
    asm("hlt");
}

// Exception 19: SIMD Floating Point Error
void simd_floating_point_error() {
    write_string_serial("SIMD Floating Point Error Occurred\n");
    asm("hlt");
}