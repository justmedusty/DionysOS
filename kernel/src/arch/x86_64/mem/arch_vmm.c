//
// Created by dustyn on 6/25/24.
//

/*
 * Taking inspo from xv6 to keep things as readable and simple as possible here.
 */

#include "include/types.h"
#include "include/mem.h"
#include "include/arch_paging.h"
#include "include/pmm.h"
#include "include/kalloc.h"
#include "include/arch_asm_functions.h"
#include "include/mem_bounds.h"
#include "include/uart.h"
#include "include/cpu.h"
#include "include/arch_vmm.h"


p4d_t *global_pg_dir = 0;
struct virt_map *kernel_pg_map;

void switch_page_table(p4d_t *page_dir) {
    lcr3((uint64)V2P(page_dir));
}

void arch_init_vmm() {
    kernel_pg_map = (struct virt_map *) P2V(phys_alloc(1));
    memset(kernel_pg_map, 0, PAGE_SIZE);
    kernel_pg_map->top_level = P2V(phys_alloc(1));
    memset(kernel_pg_map->top_level, 0, PAGE_SIZE);
    kernel_pg_map->vm_region_head = P2V(phys_alloc(1));
    memset(kernel_pg_map->vm_region_head, 0, PAGE_SIZE);

    kernel_pg_map->vm_region_head->next = kernel_pg_map->vm_region_head;
    kernel_pg_map->vm_region_head->prev = kernel_pg_map->vm_region_head;

    /*
     * Map symbols in the linker script
     */
    uint64 k_text_start = PGROUNDDOWN(&text_start);
    uint64 k_text_end = PGROUNDUP(&text_end);

    serial_printf("  Ktext start %x.64 , Ktext end %x.64\n",k_text_start,k_text_end);

    uint64 k_rodata_start = PGROUNDDOWN(&rodata_start);
    uint64 k_rodata_end = PGROUNDUP(&rodata_end);

    serial_printf("Krodata start %x.64,Krodata end %x.64\n",k_rodata_start,k_rodata_end);

    uint64 k_data_start = PGROUNDDOWN(&data_start);
    uint64 k_data_end = PGROUNDUP(&data_end);

    serial_printf("  Kdata start %x.64 , Kdata end %x.64\n",k_data_start,k_data_end);
    serial_printf("Mapping kernel text Start: %x.64 End: %x.64\n",text_start,text_end);

    if (map_pages(kernel_pg_map->top_level, (k_text_start - kernel_min) + kernel_phys_min, (uint64 *) rodata_start, 0, (uint64) text_end - (uint64) text_start) == -1) {
        panic("Mapping text!");
    }

    serial_printf("Mapping kernel rodata Start: %x.64 End: %x.64\n",rodata_start,rodata_end);

    if (map_pages(kernel_pg_map->top_level, (k_rodata_start - kernel_min) + kernel_phys_min, (uint64 *) rodata_start, PTE_NX, (uint64) rodata_end - (uint64) text_start) == -1) {
        panic("Mapping rodata!");
    }

    serial_printf("Mapping kernel data Start: %x.64 End: %x.64\n", data_start, data_end );

    if (map_pages(kernel_pg_map->top_level, (k_data_start - kernel_min) + kernel_phys_min, (uint64 *) data_start, PTE_NX | PTE_RW, (uint64) data_end - (uint64) data_start) == -1) {
        panic("Mapping data!");
    }

    /*
     * Map the first 4gb to the higher va space for the kernel, for the hhdm mapping
     */
    serial_printf("Mapping HHDM\n");
    if(map_pages(kernel_pg_map->top_level, 0 , 0 + hhdm_offset, PTE_RW | PTE_NX, 0x100000000) == -1){
        panic("Mapping first 4gb!");
    }
    struct limine_memmap_response *memmap = memmap_request.response;
    struct limine_memmap_entry **entries = memmap->entries;

    for(uint64 i = 0;i < memmap->entry_count;i++){
        uint64 base = ALIGN_DOWN(entries[i]->base,PAGE_SIZE);
        uint64 top  = ALIGN_UP(entries[i]->base + entries[i]->length,PAGE_SIZE);
        if(top < 0x100000000){
            continue;
        }
        for(uint64 j = base; j < top;j+=PAGE_SIZE ){
            if(j < (uint64)0x100000000){
                continue;
            }
           if(map_pages(kernel_pg_map->top_level, j ,j+ hhdm_offset,PTE_NX | PTE_RW,PAGE_SIZE) == -1){
               panic("hhdm mapping");
           }

        }

    }
    serial_printf("Mapped limine memory map into kernel page table\n");



    serial_printf("Kernel page table built in table located at %x.64\n", kernel_pg_map->top_level);
    lcr3(kernel_pg_map->top_level);
    serial_printf("VMM mapped and initialized");
    panic("Done");


}


/*
 * Walk a page directory to find a physical address, or allocate all the way down if the alloc flag is set
 */

static pte_t *walkpgdir(p4d_t *pgdir, const void *va, int alloc) {
    if (!pgdir) {
        return 0;
    }


    pud_t pud_idx = PUDX(va);
    pmd_t pmd_idx = PMDX(va);
    pte_t pte_idx = PTX(va);

    pud_t *pud = &pgdir[pud_idx];

    if (!(*pud & PTE_P)) {

        if (alloc) {

            *pud = (uint64) phys_alloc(1);
            memset(pud,0,PAGE_SIZE);
            *pud = (uint64) V2P(PTE_ADDR(pud)) | PTE_P | PTE_RW | PTE_U;

        } else {
            return 0;
        }
    }
    pmd_t *pmd = &pud[pmd_idx];

    if (!(*pmd & PTE_P)) {

        if (alloc) {

            *pmd = (uint64) phys_alloc(1);
            memset(pmd,0,PAGE_SIZE);
            *pmd = (uint64)  V2P(PTE_ADDR(pmd)) | PTE_P | PTE_RW | PTE_U;

        } else {
            return 0;
        }
    }

    pte_t *pte = &pmd[pte_idx];

    if (!(*pte & PTE_P)) {
        if (alloc) {
            *pte = (uint64) phys_alloc(1);
            memset(pte,0,PAGE_SIZE);
        } else {
            return 0;
        }
    }

    return pte;
}

/*
 * Maps pages from VA/PA to size in page size increments.
 */
int map_pages(p4d_t *pgdir, uint64 physaddr, uint64 *va, uint64 perms, uint64 size) {

    uint64 address, last;
    pte_t *pte;
    address = PGROUNDDOWN((uint64) va);
    last = PGROUNDDOWN(((uint64) va) + size - 1);
  
    for (;;) {
        if ((pte = walkpgdir(pgdir, (void *) address, 1)) == 0) {
            return -1;
        }

        if(*pte & PTE_P){
            /*
             * Can either keep the xv6 panic in here or can free it but will leave it for now
             */
            //serial_printf("Remap! Address being remapped is %x.64\n",*pte);
            //panic("remap");
        }

        *pte = physaddr | perms | PTE_P;

        if (address == last) {
            break;
        }

        address += PAGE_SIZE;
        physaddr += PAGE_SIZE;


    }
    return 0;
}

struct vm_region *create_region() {

}