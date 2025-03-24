//
// Created by dustyn on 12/25/24.
//
#include "stdint.h"
#include "include/system_call/system_calls.h"
#include "include/definitions/definitions.h"
int32_t system_call_dispatch(int64_t syscall_no, struct sys_call_regs args) {
    if(syscall_no < MIN_SYS || syscall_no > MAX_SYS){
        return KERN_NO_SYS;
    }

    switch (syscall_no) {
        case SYS_CLOSE:
            break;
        case SYS_CREATE:
            break;
        case SYS_WRITE:
            break;
        case SYS_READ:
            break;
        case SYS_SEEK:
            break;
        case SYS_OPEN:
            // Open a file
            break;
        case SYS_EXIT:
            break;
        case SYS_WAIT:
            break;
        case SYS_SPAWN:
            break;
        case SYS_EXEC:
            break;
        case SYS_HEAP_GROW:
            break;
        case SYS_HEAP_SHRINK:
            break;
        default:
            break;
    }
}

void register_syscall_dispatch() {
    set_syscall_handler();
}