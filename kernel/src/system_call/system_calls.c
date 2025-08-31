//
// Created by dustyn on 12/25/24.
//
#include "stdint.h"
#include "include/system_call/system_calls.h"
#include "include/definitions/definitions.h"
#include "include/scheduling/process.h"


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
    void *phys_addr = (void *)((uint64_t )pointer & 0x1000); //shed off the rest if this is some unaligned object so we can work with page aligned address
    struct process *user_process = current_process();
    phys_addr = walk_page_directory(user_process->page_map->top_level,phys_addr,0);


}

void *kernel_to_user_pointer(void *pointer){
    struct process *user_process = my_cpu()->running_process;
    uint64_t phys_addr = (uint64_t)Virt2Phys(pointer);

    /*
     * We are just going to default to page size for now if this is ever an issue we can come back to this.
     */
    arch_map_pages(my_cpu()->page_map->top_level,phys_addr, Virt2Phys(phys_addr),READWRITE | NO_EXECUTE,PAGE_SIZE);

    return pointer;
}