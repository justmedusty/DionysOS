//
// Created by dustyn on 12/25/24.
//

#ifndef SYSTEM_CALLS_H
#define SYSTEM_CALLS_H

#include "stdint.h"

/*
 * x86_64 requires writing to the MSR defined below the address of the syscall handler so that when syscall instruction
 * is used in userspace after CS changes to ring 0, it will jump to the handler we set here
 */
#ifdef __x86_64__

#include "include/architecture/x86_64/msr.h"

struct regs {
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t r10;
    uint64_t r8;
    uint64_t r9;
};
#define IA32_LSTAR 0xC0000082
extern int syscall_entry();
static inline void set_syscall_handler() {
    wrmsr(IA32_LSTAR, (uint64_t) syscall_entry);
}

#endif


int32_t system_call_dispatch();
void register_syscall_dispatch();

enum system_calls {
    SYS_WRITE,
    SYS_READ,
    SYS_SEEK,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_EXIT,
    SYS_WAIT,
    SYS_SPAWN,
    SYS_EXEC,
    SYS_CREATE,
    SYS_HEAP_GROW,
    SYS_HEAP_SHRINK,
};
#endif //SYSTEM_CALLS_H
