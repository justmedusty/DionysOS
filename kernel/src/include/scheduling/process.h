//
// Created by dustyn on 9/14/24.
//

#pragma once
#include "include/arch/arch_cpu.h"
#include "include/arch/arch_vmm.h"
#include "include/scheduling/process.h"

/*
 *	Process states
 */

#define RUNNING_STATE 0
#define READY_STATE 1
#define SLEEPING_STATE 2
#define DEBUG_STATE 3

/*
 *  Process types
 */

#define KERNEL_THREAD 0
#define USER_THREAD 1
#define USER_PROCESS 2

//4 pages
#define DEFAULT_STACK_SIZE 0x8000



struct process {
    uint8_t current_state;
    uint8_t priority;
    uint16_t process_id;
    uint16_t parent_process_id;
    uint64_t signal; /* This will probably end up being some sort of queue but I will put this here for now */
    uint64_t signal_mask;
    uint64_t time_quantum;
    uint64_t ticks_taken; /* How many timer ticks has this process ran ? Will inherently be somewhat approximate since it won't know half ticks, quarter ticks etc*/
    uint64_t process_type;
    uint16_t file_descriptors[16];
    void *sleep_channel;
    struct virtual_handle_list *handle_list;
    struct virt_map* page_map;
    struct cpu* current_cpu; /* Which run queue , if any is this process on? */
    struct gpr_state* current_gpr_state;
    struct vnode* current_working_dir;
};

#ifdef __x86_64__

struct gpr_state {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rip;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
};

#endif
