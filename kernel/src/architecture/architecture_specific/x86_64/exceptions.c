//
// Created by dustyn on 6/19/24.
//
#pragma once
#include <include/architecture/arch_cpu.h>
#include <include/drivers/display/framebuffer.h>
#include <include/scheduling/sched.h>

#include "include/architecture/x86_64/asm_functions.h"

#include "include/definitions/types.h"
#include "include/architecture/arch_cpu.h"
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
    panic("Non-Maskable Interrupt Occurred\n");
    
}

// Exception 3: Breakpoint
void breakpoint() {
    panic("Breakpoint Occurred\n");
}

// Exception 4: Overflow
void overflow() {
    panic("Overflow Occurred\n");
    
}

// Exception 5: Bounds Check
void bounds_check() {
    panic("Bounds Check Exception Occurred\n");
}

// Exception 6: Illegal Opcode
void illegal_opcode() {
    panic("Illegal Opcode Exception Occurred\n");
    
}

// Exception 7: Device Not Available
void device_not_available() {
    panic("Device Not Available Exception Occurred\n");
    
}

// Exception 8: Double Fault
void double_fault() {
    panic("Double Fault Occurred\n");
    
}

// Exception 10: Invalid Task Switch Segment
void invalid_tss() {
    panic("Invalid Task Switch Segment Occurred\n");
    
}

// Exception 11: Segment Not Present
void segment_not_present() {
    panic("Segment Not Present Exception Occurred\n");
    
}

// Exception 12: Stack Exception
void stack_exception() {
    panic("Stack Exception Occurred\n");
    
}

// Exception 13: General Protection Fault
void general_protection_fault(int32_t error_code) {
    err_printf("Error Code %i\n",error_code);
    panic("General Protection Fault Occurred");
}

// Exception 14: Page Fault
void page_fault() {
    uint64_t faulting_address = rcr2();
    uint64_t page_map = rcr3();
    uint64_t cpu_no = my_cpu()->cpu_number;
    err_printf("Page Fault Occurred With Access %x.64 within page map %x.64 on CPU %i\n", faulting_address,page_map,cpu_no);
    if (IS_USER_ADDRESS(faulting_address) && my_cpu()->running_process->process_type != KERNEL_THREAD) {
        sched_exit();
    }
    panic("Page Fault!");
}

// Exception 16: Floating Point Error
void floating_point_error() {
    panic("Floating Point Error Occurred\n");
}

// Exception 17: Alignment Check
void alignment_check() {
    panic("Alignment Check Occurred\n");
}

// Exception 18: Machine Check
void machine_check() {
    panic("Machine Check Occurred\n");
    
}

// Exception 19: SIMD Floating Point Error
void simd_floating_point_error() {
    panic("SIMD Floating Point Error Occurred\n");
    
}
