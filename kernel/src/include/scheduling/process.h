//
// Created by dustyn on 9/14/24.
//

#pragma once
#include "include/types.h"
#include "include/arch/arch_vmm.h"
#include "include/arch/arch_cpu.h"
#include "include/scheduling/process.h"

/*
 *	Process states
 */

#define RUNNING_STATE 0
#define READY_STATE 1
#define SLEEPING_STATE 2
#define DEBUG_STATE 3


struct process {
    uint8 current_state;
    uint8 priority;
    uint64 time_quantum;
    uint64 ticks_taken; /* How many timer ticks has this process ran ? Will inherently be somewhat approximate since it won't know half ticks, quarter ticks etc*/
    uint16 process_id;
    uint16 parent_process_id;
    uint16 signal; /* This will probably end up being some sort of queue but I will put this here for now */
    void* kernel_stack;
    struct page_map* page_map;
    struct cpu* current_cpu; /* Which run queue , if any is this process on? */
    struct gpr_state* current_gpr_state;
};


#ifdef __x86_64__

struct gpr_state {
    uint64 rax;
    uint64 rbx;
    uint64 rcx;
    uint64 rdx;
    uint64 rdi;
    uint64 rsi;
    uint64 rbp;
    uint64 rsp;
    uint64 rip;
    uint64 r8;
    uint64 r9;
    uint64 r10;
    uint64 r11;
    uint64 r12;
    uint64 r13;
    uint64 r14;
    uint64 r15;
};

#endif
