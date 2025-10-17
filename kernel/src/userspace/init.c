//
// Created by dustyn on 7/7/25.
//

#include "systemcall_wrappers.h"

int main(int argc, char *argv[]) {
    int i = 2 + 2;
    sys_exit();
    int64_t handle = sys_open("/dev/FRAMEBUFFER0");

    if (handle < 0) {
        sys_exit();
    }

    sys_write(handle, "Hello World!\n", 13);
    sys_exit();
}
