//
// Created by dustyn on 9/14/24.
//

#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "include/filesystem/vfs.h"
#include <include/memory/kmalloc.h>
#include "include/architecture/arch_cpu.h"
#include "include/architecture/arch_vmm.h"
#include "include/scheduling/process.h"

/*
 *	Process states
 */

enum process_state {
    PROCESS_RUNNING,
    PROCESS_SLEEPING,
    PROCESS_DEAD,
    DEBUG_STATE,
    PROCESS_READY
};


/*
 *  Process types
 */
enum process_type {
    KERNEL_THREAD,
    USER_THREAD,
    USER_PROCESS
};


//16 pages
#define DEFAULT_STACK_SIZE 0x16000

struct process {
    uint8_t current_state;
    uint8_t priority;
    uint8_t effective_priority; // Going to use a base priority and an effective priority similar to the real and effective uid idea in linux so that you can promote processes who are being passed over.
    uint16_t process_id;
    uint16_t parent_process_id;
    uint64_t signal; /* This will probably end up being some sort of queue, but I will put this here for now */
    uint64_t signal_mask;
    uint64_t time_quantum;
    uint64_t ticks_taken;
    uint64_t ticks_slept;/* How many timer ticks has this process ran ? Will inherently be somewhat approximate since it won't know half ticks, quarter ticks etc*/
    uint8_t process_type;
    uint64_t start_time;
    uint8_t file_descriptors[16];
    bool inside_kernel;
    void *stack;
    void *kernel_stack;
    void *sleep_channel;
    struct virtual_handle_list *handle_list;
    struct virt_map *page_map;
    struct cpu *current_cpu; /* Which run queue , if any is this process on? */
    struct register_state *current_register_state;
    struct vnode *current_working_dir;
};


struct register_state {
#ifdef __x86_64__
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
#endif
    bool interrupts_enabled;
};


#endif

void exit();
void sleep(void *channel);
void wakeup(void *channel);
void switch_kernel_stack(struct process *incoming_process);