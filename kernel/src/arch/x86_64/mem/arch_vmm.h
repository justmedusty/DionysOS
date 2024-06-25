//
// Created by dustyn on 6/25/24.
//

#ifndef KERNEL_ARCH_VMM_H
#define KERNEL_ARCH_VMM_H
#pragma once

static inline void __native_flush_tlb_single(unsigned long addr) {
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}
#endif //KERNEL_ARCH_VMM_H
