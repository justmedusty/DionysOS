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
    lcr3(page_dir);
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
    uint64 k_text_start = ALIGN_DOWN((uint64 *) (void*)text_start, PAGE_SIZE);
    uint64 k_text_end = ALIGN_UP((uint64 *) (void*)text_end, PAGE_SIZE);

    serial_printf("  Ktext start %x.64 , Ktext end %x.64\n",k_text_start,k_text_end);

    uint64 k_rodata_start = ALIGN_DOWN((uint64 *) (void*)rodata_start, PAGE_SIZE);
    uint64 k_rodata_end = ALIGN_UP((uint64 *) (void*)rodata_end, PAGE_SIZE);

    serial_printf("Krodata start %x.64,Krodata end %x.64\n",k_rodata_start,k_rodata_end);

    uint64 k_data_start = ALIGN_DOWN((uint64 *) (void*)data_start, PAGE_SIZE);
    uint64 k_data_end = ALIGN_UP((uint64 *) (void*)data_end, PAGE_SIZE);

    serial_printf("  Kdata start %x.64 , Kdata end %x.64\n",k_data_start,k_data_end);

    serial_printf("Mapping kernel text Start: %x.64 End: %x.64\n",k_text_start,k_text_end);
    if(k_text_start > k_text_end){
        panic("Start higher than end");
    }
    panic("");
    if (map_pages(kernel_pg_map->top_level, k_text_start - kernel_min + kernel_phys_min, k_text_start, 0, k_text_end - k_text_start) == -1) {
        panic("Mapping text!");
    }
    panic("done");
    serial_printf("Mapping kernel rodata Start: %x.64 End: %x.64\n",k_rodata_start,(k_rodata_start - kernel_min + kernel_phys_min) + (k_rodata_end - k_rodata_start));

    if (map_pages(kernel_pg_map->top_level, V2P(k_rodata_start),P2V(k_rodata_start), PTE_NX, k_rodata_end - k_text_start) == -1) {
        panic("Mapping rodata!");
    }

    serial_printf("Mapping kernel data Start: %x.64 End: %x.64\n",k_data_start - kernel_min + kernel_phys_min,(k_data_start - kernel_min + kernel_phys_min) + (k_data_end - k_data_start) );
    if (map_pages(kernel_pg_map->top_level, k_data_start - kernel_min + kernel_phys_min,k_data_start, PTE_NX | PTE_RW, k_data_end - k_data_start) == -1) {
        panic("Mapping data!");
    }

    /*
     * Map the first 4gb to the higher va space for the kernel, for the hhdm mapping
     */
    if(map_pages(kernel_pg_map->top_level, 0, 0, PTE_P | PTE_RW, 0xFFFFFFFF) == -1){
        panic("Mapping first 4gb!");
    }

    serial_printf("Kernel page table built\n");


}


/*
 * Walk a page directory to find a physical address, or allocate all the way down if the alloc flag is set
 */

static pte_t *walkpgdir(p4d_t *pgdir, const uint64 *va, int alloc) {
    serial_printf("va is %x.64\n",va);
    if (!pgdir) {
        return 0;
    }

    pte_t result;

    pud_t pud_idx = PUDX(va);
    pmd_t pmd_idx = PMDX(va);
    pte_t pte_idx = PTX(va);

    pud_t *pud = &pgdir[pud_idx];

    if (!(*pud & PTE_P)) {

        if (alloc) {

            *pud = (uint64) kalloc(PAGE_SIZE);
            memset(*pud,0,PAGE_SIZE);
            *pud = (uint64) P2V(pud) | PTE_P | PTE_RW | PTE_U;

        } else {

            return 0;

        }
    }
    pmd_t *pmd = &pud[pmd_idx];

    if (!(*pmd & PTE_P)) {

        if (alloc) {

            *pmd = (uint64) kalloc(PAGE_SIZE);
            memset(*pmd,0,PAGE_SIZE);
            *pmd = (uint64) P2V(pmd) | PTE_P | PTE_RW | PTE_U;

        } else {

            return 0;

        }
    }

    pte_t *pte = &pmd[pte_idx];

    if (!(*pte & PTE_P)) {
        if (alloc) {
            *pte = (uint64) kalloc(PAGE_SIZE);
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


    uint64 *address, *last;
    pte_t *pte;
    address = PGROUNDDOWN((uint64) va);
    last = PGROUNDDOWN(((uint64) va) + size - 1);
    serial_printf("address : %x.64 last : %x.64 size : %x.6\n",address,last,size);
    panic("");

    for (;;) {
        if ((pte = walkpgdir(pgdir, address, 1)) == 0) {
            return -1;
        }

        if(*pte & PTE_P){
            serial_printf("Remap! Address being remapped is %x.64\n",*pte);
            //panic("remap");
        }

        *pte = physaddr | perms | PTE_P;

        if (address == last) {
            break;
        }

        address += PAGE_SIZE;
        physaddr += PAGE_SIZE;


    }
    panic("dopne");
    return 0;
}

struct vm_region *create_region() {

}