//
// Created by dustyn on 12/25/24.
//
#include "stdint.h"
#include "include/system_call/system_calls.h"
#include "include/definitions/definitions.h"


int64_t system_call_dispatch(int64_t syscall_no, struct syscall_args *args) {
    if (syscall_no < MIN_SYS || syscall_no > MAX_SYS) {
        return KERN_NO_SYS;
    }


    switch (syscall_no) {
        case SYS_CLOSE:
            close(args->arg1);
            return KERN_SUCCESS;
        case SYS_CREATE:
            return create((char *) args->arg1, (char *) args->arg2, args->arg3);
        case SYS_WRITE:
            return write(args->arg1, (char *) args->arg2, args->arg3);
        case SYS_READ:
            return read(args->arg1, (char *) args->arg2, args->arg3);
        case SYS_SEEK:
            return seek(args->arg1, args->arg2);
        case SYS_OPEN:
            return open((char *) args->arg1);
        case SYS_MOUNT:
            return mount((char *) args->arg1, (char *) args->arg2);
        case SYS_UNMOUNT:
            return unmount((char *) args->arg1);
        case SYS_RENAME:
            return rename((char *) args->arg1, (char *) args->arg2);
        case SYS_EXIT:
            exit();
            return KERN_SUCCESS;
        default:
            return KERN_NO_SYS; // Return an error for unknown syscalls
    }
}

void register_syscall_dispatch() {
    set_syscall_handler();
}

void *user_to_kernel_pointer(void *pointer){
    //TODO
}

void *kernel_to_user_pointer(void *pointer){
    //TODO
}