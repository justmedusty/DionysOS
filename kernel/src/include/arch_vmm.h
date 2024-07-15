//
// Created by dustyn on 6/25/24.
//

#pragma once

#define ALIGN_UP(value,aligned_to) ((value+ aligned_to(aligned_to-1)) / aligned_to) * aligned_to
#define ALIGN_DOWN(value,aligned_to) (value /aligned_to) * aligned_to

static inline void native_flush_tlb_single(unsigned long vaddr) {
    asm volatile("invlpg (%0)" ::"r" (vaddr) : "memory");
}

static inline void native_flush_tlb_range(unsigned long vaddr,uint64 pages) {
    for(int i = 0; i < pages; i++){
        asm volatile("invlpg (%0)" ::"r" (vaddr + (i * PAGE_SIZE)) : "memory");
    }
}

typedef struct virtual_region {
    uint64 va;
    uint64 pa;
    uint64 end_addr;
    uint64 num_pages;
    uint64 flags;

    uint64 ref_count;

    struct virtual_region* next;
    struct virtual_region* prev;

} virtual_region;

typedef struct virt_map {
    uint64 *top_level;
    struct virtual_region *vm_region_head;
};

extern char text_start[];
extern char text_end[];
extern char rodata_start[];
extern char rodata_end[];
extern char data_start[];
extern char data_end[];

void arch_init_vmm();
void switch_page_table(p4d_t *page_dir);
int map_pages(p4d_t *pgdir, uint64 physaddr, uint64 *va, uint32 perms,uint64 size);
