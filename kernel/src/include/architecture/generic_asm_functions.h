//
// Created by dustyn on 9/17/24.
//

#pragma once
#include "include/definitions/types.h"

#ifdef __x86_64__
#include "include/architecture/x86_64/asm_functions.h"

static inline void write_port(uint16_t port, uint8_t value) {
      outb(port, value);
}

static inline uint8_t read_port(uint16_t port) {
      return inb(port);
}

static inline void flush_cache(void * address){
    clflush64(address);
}

#endif