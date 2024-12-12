//
// Created by dustyn on 8/30/24.
//
#pragma once

#include <include/mem/kalloc.h>
#ifdef __x86_64__
#pragma once
#include "include/arch/x86_64/asm_functions.h"
#include "include/scheduling/process.h"

static inline void enable_interrupts() {
  cli();
}

static inline void disable_interrupts() {
  sti();
}

static inline uint64_t are_interrupts_enabled() {
  return interrupts_enabled();
}


#endif


static inline void try_change_interrupt_flag(uint64_t interrupt_bit) {
  if (are_interrupts_enabled() != interrupt_bit) {
    switch (interrupt_bit) {
    case 0:
      disable_interrupts();
      return;
    case 1: enable_interrupts();
      return;
    default:
      panic("try_change_interrupt_flag bad value passed");
    }
  }
}