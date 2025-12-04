//
// Created by dustyn on 7/7/25.
//
#include <stdint.h>
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


#ifdef __x86_64__
int64_t syscall_stub(
    uint64_t syscall_no,
    uint64_t arg1, uint64_t arg2, uint64_t arg3,
    uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
    int64_t ret;
    __asm__ __volatile__ (
        "mov %[arg4], %%r10\n\t"
        "mov %[arg5], %%r8\n\t"
        "mov %[arg6], %%r9\n\t"
        "syscall"
        : "=a"(ret)
        : "a"(syscall_no),
          "D"(arg1), "S"(arg2), "d"(arg3),
          [arg4]"r"(arg4), [arg5]"r"(arg5), [arg6]"r"(arg6)
        : "r10", "r8", "r9", "rcx", "r11", "memory"
    );
    return ret;
}

#endif

int64_t sys_open(char *path) {
    return syscall_stub((uint64_t)SYS_OPEN, (uint64_t)path, 0, 0, 0, 0, 0);
}

int64_t sys_write(uint64_t handle, char *buffer, uint64_t bytes) {
    return syscall_stub((uint64_t)SYS_WRITE, handle, (uint64_t)buffer, bytes, 0, 0, 0);
}

int64_t sys_read(uint64_t handle, char *buffer, uint64_t bytes) {
    return syscall_stub((uint64_t)SYS_READ, handle, (uint64_t)buffer, bytes, 0, 0, 0);
}

int64_t sys_close(uint64_t handle) {
    return syscall_stub((uint64_t)SYS_CLOSE, handle, 0, 0, 0, 0, 0);
}

int64_t sys_mount(char *mount_point, char *mounted_filesystem) {
    return syscall_stub((uint64_t)SYS_MOUNT, (uint64_t)mount_point, (uint64_t)mounted_filesystem, 0, 0, 0, 0);
}

int64_t sys_unmount(char *path) {
    return syscall_stub((uint64_t)SYS_UNMOUNT, (uint64_t)path, 0, 0, 0, 0, 0);
}

int64_t sys_rename(char *path, char *new_name) {
    return syscall_stub((uint64_t)SYS_RENAME, (uint64_t)path, (uint64_t)new_name, 0, 0, 0, 0);
}

int64_t sys_create(char *path, char *name, uint64_t type) {
    return syscall_stub((uint64_t)SYS_CREATE, (uint64_t)path, (uint64_t)name, type, 0, 0, 0);
}

int64_t sys_seek(uint64_t handle, uint64_t whence) {
    return syscall_stub((uint64_t)SYS_SEEK, handle, whence, 0, 0, 0, 0);
}

int64_t sys_exit() {
    return syscall_stub((uint64_t)SYS_EXIT, 0, 0, 0, 0, 0, 0);
}

int main(int argc, char *argv[]) {
    int i = 2 + 2;
    int64_t handle = sys_open("/dev/FRAMEBUFFER0");

    if (handle < 0) {
        sys_exit();
        for (;;) { __asm__ __volatile__("pause"); }
    }

    sys_write(handle, "Hello World!\n", 13);
    sys_exit();
    for (;;) { __asm__ __volatile__("pause"); }
}
