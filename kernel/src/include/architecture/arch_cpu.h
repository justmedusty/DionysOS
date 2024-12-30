//
// Created by dustyn on 8/11/24.
//

#pragma once
#include "include/memory/vmm.h"
#include "include/definitions/types.h"
#include "limine.h"
#include "include/scheduling/process.h"
#include "include/data_structures/queue.h"

//Static allocation for 8, will never use this many but that is okay.


#ifdef __x86_64__

typedef struct cpu_state {
    uint64_t ds;
    uint64_t es;
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t err;
    uint64_t ss;
} cpu_state;


struct cpu{
    uint8_t cpu_number;
    uint32_t lapic_id;
    uint64_t lapic_timer_frequency;
    cpu_state* cpu_state;
    struct gpr_state *scheduler_state;
    struct tss* tss;
    struct virt_map* page_map;
    struct process* running_process;
    struct queue* local_run_queue;
};



#endif


/*
 *  Function prototypes
 */
#ifndef MAX_CPUS
#define MAX_CPUS 2 // this will be overridden by the nproc return value
#endif
extern struct spinlock bootstrap_lock;
//static data structure for now this all just chicken scratch for the time being but I don't see a point of a linked list for cpus since it will never be more than 4 probably
extern struct cpu cpu_list[MAX_CPUS];
extern struct queue local_run_queues[MAX_CPUS];

void panic(const char* str);
struct cpu* my_cpu();
struct process* current_process();
void arch_initialise_cpu(struct limine_smp_info* smp_info);
// For other processors panicking the next PIT interrupt
extern uint8_t panicked;
