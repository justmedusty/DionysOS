//
// Created by dustyn on 6/25/24.
//

#ifndef KERNEL_ARCH_VMM_H
#define KERNEL_ARCH_VMM_H
#pragma once

static inline void __native_flush_tlb_single(unsigned long addr) {
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}
void arch_init_vmm();
void switch_page_table(p4d_t *page_dir);
void map_pages(p4d_t *pgdir, uint64 physaddr, uint64 *va, uint32 perms,uint64 size);
#endif //KERNEL_ARCH_VMM_H
