//
// Created by dustyn on 8/30/24.
//

#ifdef __x86_64__
#pragma once
#include "include/arch/x86_64/asm_functions.h"

void enable_interrupts(){
  cli();
}

void disable_interrupts(){
  sti();
}

#endif
