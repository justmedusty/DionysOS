//
// Created by dustyn on 8/30/24.
//
#pragma once
#include <include/mem/kalloc.h>
#ifdef __x86_64__
#pragma once
#include "include/arch/x86_64/asm_functions.h"
#include "include/scheduling/process.h"
extern void get_regs();
static inline void enable_interrupts() {
  cli();
}

static inline void disable_interrupts() {
  sti();
}


static inline void get_gpr_state(struct gpr_state* gpr) {
  get_regs(gpr);
}
#endif
