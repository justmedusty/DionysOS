//
// Created by dustyn on 9/28/25.
//
#include <stdint.h>
uint64_t get_current_stack() {
    uint64_t rsp;
    __asm__ volatile ("mov %%rsp, %0" : "=r"(rsp));
    return rsp;
}

uint64_t get_current_base() {
    uint64_t rbp;
    __asm__ volatile ("mov %%rbp, %0" : "=r"(rbp));
    return rbp;
}
