//
// Created by dustyn on 3/25/25.
//

#ifndef KERNEL_SYSTEMCALL_WRAPPERS_H
#define KERNEL_SYSTEMCALL_WRAPPERS_H
#pragma once
#include "stdint.h"
#include "systemcall_wrappers.h"

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


int syscall_stub(uint64_t syscall_number, uint64_t arg1,uint64_t arg2,uint64_t arg3,uint64_t arg4,uint64_t arg5,uint64_t arg6);


static inline int64_t sys_open(char *path) {
    return syscall_stub((uint64_t)SYS_OPEN, (uint64_t)path, 0, 0, 0, 0, 0);
}

static inline int64_t sys_write(uint64_t handle, char *buffer, uint64_t bytes) {
    return syscall_stub((uint64_t)SYS_WRITE, handle, (uint64_t)buffer, bytes, 0, 0, 0);
}

static inline int64_t sys_read(uint64_t handle, char *buffer, uint64_t bytes) {
    return syscall_stub((uint64_t)SYS_READ, handle, (uint64_t)buffer, bytes, 0, 0, 0);
}

static inline int64_t sys_close(uint64_t handle) {
    return syscall_stub((uint64_t)SYS_CLOSE, handle, 0, 0, 0, 0, 0);
}

static inline int64_t sys_mount(char *mount_point, char *mounted_filesystem) {
    return syscall_stub((uint64_t)SYS_MOUNT, (uint64_t)mount_point, (uint64_t)mounted_filesystem, 0, 0, 0, 0);
}

static inline int64_t sys_unmount(char *path) {
    return syscall_stub((uint64_t)SYS_UNMOUNT, (uint64_t)path, 0, 0, 0, 0, 0);
}

static inline int64_t sys_rename(char *path, char *new_name) {
    return syscall_stub((uint64_t)SYS_RENAME, (uint64_t)path, (uint64_t)new_name, 0, 0, 0, 0);
}

static inline int64_t sys_create(char *path, char *name, uint64_t type) {
    return syscall_stub((uint64_t)SYS_CREATE, (uint64_t)path, (uint64_t)name, type, 0, 0, 0);
}

static inline int64_t sys_seek(uint64_t handle, uint64_t whence) {
    return syscall_stub((uint64_t)SYS_SEEK, handle, whence, 0, 0, 0, 0);
}

static inline int64_t sys_exit() {
    return syscall_stub((uint64_t)SYS_EXIT, 0, 0, 0, 0, 0, 0);
}
#endif //KERNEL_SYSTEMCALL_WRAPPERS_H
