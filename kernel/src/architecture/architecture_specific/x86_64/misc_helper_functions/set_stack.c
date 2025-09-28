//
// Created by dustyn on 9/28/25.
//
#include <stdint.h>

__attribute__((naked)) void set_current_stack(uint64_t rsp) {
    __asm__ volatile (
        "mov %rdi, %rsp\n"
        "ret"
    );
}