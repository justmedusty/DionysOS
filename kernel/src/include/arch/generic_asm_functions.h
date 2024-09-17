//
// Created by dustyn on 9/17/24.
//

#pragma once
#include "include/types.h"

#ifdef __x86_64__
#include "include/arch/x86_64/asm_functions.h"

static inline void write_port(uint16 port, uint8 value) {
      outb(port, value);
}

static inline uint8 read_port(uint16 port) {
      return inb(port);
}

#endif