//
// Created by dustyn on 3/25/25.
//
#include "systemcall_wrappers.h"
#include "include/system_call/system_calls.h"


static inline int64_t open(char *path) {
    return syscall_stub((uint64_t)SYS_OPEN, (uint64_t)path, 0, 0, 0, 0, 0);
}

static inline int64_t write(uint64_t handle, char *buffer, uint64_t bytes) {
return syscall_stub((uint64_t)SYS_WRITE, handle, (uint64_t)buffer, bytes, 0, 0, 0);
}

static inline int64_t read(uint64_t handle, char *buffer, uint64_t bytes) {
return syscall_stub((uint64_t)SYS_READ, handle, (uint64_t)buffer, bytes, 0, 0, 0);
}

static inline int64_t close(uint64_t handle) {
return syscall_stub((uint64_t)SYS_CLOSE, handle, 0, 0, 0, 0, 0);
}

static inline int64_t mount(char *mount_point, char *mounted_filesystem) {
    return syscall_stub((uint64_t)SYS_MOUNT, (uint64_t)mount_point, (uint64_t)mounted_filesystem, 0, 0, 0, 0);
}

static inline int64_t unmount(char *path) {
    return syscall_stub((uint64_t)SYS_UNMOUNT, (uint64_t)path, 0, 0, 0, 0, 0);
}

static inline int64_t rename(char *path, char *new_name) {
    return syscall_stub((uint64_t)SYS_RENAME, (uint64_t)path, (uint64_t)new_name, 0, 0, 0, 0);
}

static inline int64_t create(char *path, char *name, uint64_t type) {
    return syscall_stub((uint64_t)SYS_CREATE, (uint64_t)path, (uint64_t)name, type, 0, 0, 0);
}

static inline int64_t seek(uint64_t handle, uint64_t whence) {
return syscall_stub((uint64_t)SYS_SEEK, handle, whence, 0, 0, 0, 0);
}

static inline int64_t exit() {
return syscall_stub((uint64_t)SYS_EXIT, 0, 0, 0, 0, 0, 0);
}