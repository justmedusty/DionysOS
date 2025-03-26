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

struct syscall_args {
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
    uint64_t arg6;
};
#define IA32_LSTAR 0xC0000082

__attribute__((always_inline))
extern int syscall_entry();

static inline void set_syscall_handler() {
    wrmsr(IA32_LSTAR, (uint64_t) syscall_entry);
}

#endif


int64_t system_call_dispatch();

void register_syscall_dispatch();

enum system_calls {
    MIN_SYS,
    SYS_WRITE,
    SYS_READ,
    SYS_SEEK,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_MOUNT,
    SYS_UNMOUNT,
    SYS_RENAME,
    SYS_EXIT,
    SYS_WAIT,
    SYS_SPAWN,
    SYS_EXEC,
    SYS_CREATE,
    SYS_HEAP_GROW,
    SYS_HEAP_SHRINK,
    MAX_SYS
};
#endif //SYSTEM_CALLS_H
