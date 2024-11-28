//
// Created by dustyn on 6/19/24.
//
#pragma once
#include <include/arch/arch_cpu.h>
#include "include/arch/x86_64/asm_functions.h"

#include "include/types.h"
#include "include/arch/arch_cpu.h"
#include "include/drivers/serial/uart.h"

//Exception 0
void divide_by_zero() {
    panic("Divide by Zero Occurred");
}

//Exception 1
void debug_exception() {
    panic("Debug Exception Occurred");
}

// Exception 2: Non-Maskable Interrupt
void nmi_interrupt() {
    serial_printf("Non-Maskable Interrupt Occurred\n");
    asm("hlt");
}

// Exception 3: Breakpoint
void breakpoint() {
    serial_printf("Breakpoint Occurred\n");
}

// Exception 4: Overflow
void overflow() {
    serial_printf("Overflow Occurred\n");
    asm("hlt");
}

// Exception 5: Bounds Check
void bounds_check() {
    serial_printf("Bounds Check Exception Occurred\n");
}

// Exception 6: Illegal Opcode
void illegal_opcode() {
    serial_printf("Illegal Opcode Exception Occurred\n");
    asm("hlt");
}

// Exception 7: Device Not Available
void device_not_available() {
    serial_printf("Device Not Available Exception Occurred\n");
    asm("hlt");
}

// Exception 8: Double Fault
void double_fault() {
    serial_printf("Double Fault Occurred\n");
    asm("hlt");
}

// Exception 10: Invalid Task Switch Segment
void invalid_tss() {
    serial_printf("Invalid Task Switch Segment Occurred\n");
    asm("hlt");
}

// Exception 11: Segment Not Present
void segment_not_present() {
    serial_printf("Segment Not Present Exception Occurred\n");
    asm("hlt");
}

// Exception 12: Stack Exception
void stack_exception() {
    serial_printf("Stack Exception Occurred\n");
    asm("hlt");
}

// Exception 13: General Protection Fault
void general_protection_fault(int32_t error_code) {
    serial_printf("Error Code %i\n",error_code);
    panic("General Protection Fault Occurred");
    asm("hlt");
}

// Exception 14: Page Fault
void page_fault() {
    uint64_t faulting_address = rcr2();
    serial_printf("Page Fault Occurred With Access %x.64\n", faulting_address);
    panic("Page Fault!");
}

// Exception 16: Floating Point Error
void floating_point_error() {
    serial_printf("Floating Point Error Occurred\n");
    asm("hlt");
}

// Exception 17: Alignment Check
void alignment_check() {
    serial_printf("Alignment Check Occurred\n");
    asm("hlt");
}

// Exception 18: Machine Check
void machine_check() {
    serial_printf("Machine Check Occurred\n");
    asm("hlt");
}

// Exception 19: SIMD Floating Point Error
void simd_floating_point_error() {
    serial_printf("SIMD Floating Point Error Occurred\n");
    asm("hlt");
}
