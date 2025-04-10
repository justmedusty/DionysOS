//
// Created by dustyn on 3/25/25.
//

#ifndef KERNEL_SYSTEMCALL_WRAPPERS_H
#define KERNEL_SYSTEMCALL_WRAPPERS_H
#pragma once
#include "stdint.h"
__attribute__((always_inline))
int syscall_stub(uint64_t syscall_number, uint64_t arg1,uint64_t arg2,uint64_t arg3,uint64_t arg4,uint64_t arg5,uint64_t arg6);
#endif //KERNEL_SYSTEMCALL_WRAPPERS_H
