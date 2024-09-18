//
// Created by dustyn on 8/11/24.
//

#pragma once
#include "include/mem/vmm.h"
#include "include/types.h"
#include "limine.h"
#include "include/scheduling/process.h"
#include "include/data_structures/queue.h"

//Static allocation for 8, will never use this many but that is okay.


#ifdef __x86_64__

typedef struct cpu_state {
    uint64 ds;
    uint64 es;
    uint64 rax;
    uint64 rbx;
    uint64 rcx;
    uint64 rdx;
    uint64 rdi;
    uint64 rsi;
    uint64 rbp;
    uint64 rsp;
    uint64 rip;
    uint64 cs;
    uint64 rflags;
    uint64 r8;
    uint64 r9;
    uint64 r10;
    uint64 r11;
    uint64 r12;
    uint64 r13;
    uint64 r14;
    uint64 r15;
    uint64 err;
    uint64 ss;
} cpu_state;


typedef struct {
    uint8 cpu_number;
    uint32 lapic_id;
    uint64 lapic_timer_frequency;
    cpu_state* cpu_state;
    struct tss* tss;
    struct virt_map* page_map;
    struct process* running_process;
    struct queue* local_run_queue;
} cpu;



#endif


/*
 *  Function prototypes
 */

extern struct spinlock bootstrap_lock;
//static data structure for now this all just chicken scratch for the time being but I don't see a point of a linked list for cpus since it will never be more than 4 probably
extern cpu cpu_list[8];
extern struct queue local_run_queues[8];

void panic(const char* str);
cpu* mycpu();
struct process* current_process();
void arch_initialise_cpu(struct limine_smp_info* smp_info);
// For other processors panicking the next PIT interrupt
extern uint8 panicked;
