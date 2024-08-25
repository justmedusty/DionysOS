//
// Created by dustyn on 8/11/24.
//

#pragma once
#include "include/types.h"


void arch_panic(const char *str);
uint64 arch_mycpu();

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


typedef struct local_cpu{
  uint8 cpu_number;
  struct cpu_state *cpu_state;
  struct tss *tss;
  uint32 lapic_id;
  uint64 lapic_timer_frequency;
} local_cpu;


extern local_cpu local_cpu_info[16];
