//
// Created by dustyn on 8/30/24.
//
#pragma once
#ifdef __x86_64__
#pragma once
#include "include/arch/x86_64/asm_functions.h"

static inline void enable_interrupts(){
  cli();
}

static inline void disable_interrupts(){
  sti();
}
#endif
