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

void switch_page_table(p4d_t *page_dir){
    lcr3(page_dir);
}

void arch_init_vmm(){

    kernel_pg_map = (struct virt_map*)P2V(phys_alloc(1));
    memset(kernel_pg_map,0,PAGE_SIZE);
    kernel_pg_map->top_level = P2V(phys_alloc(1));
    memset(kernel_pg_map->top_level,0,PAGE_SIZE);
    kernel_pg_map->vm_region_head = P2V(phys_alloc(1));
    memset(kernel_pg_map->vm_region_head,0,PAGE_SIZE);

    kernel_pg_map->vm_region_head->next = kernel_pg_map->vm_region_head;
    kernel_pg_map->vm_region_head->prev = kernel_pg_map->vm_region_head;

    /*
     * Map symbols in the linker script
     */
    uint64 k_text_start = ALIGN_DOWN(text_start,PAGE_SIZE);
    uint64 k_text_end = ALIGN_UP(text_end,PAGE_SIZE);

    uint64 k_rodata_start =  ALIGN_DOWN(rodata_start,PAGE_SIZE);
    uint64 k_rodata_end = ALIGN_UP(rodata_end,PAGE_SIZE);

    uint64 k_data_start = ALIGN_DOWN(data_start,PAGE_SIZE);
    uint64 k_data_end = ALIGN_UP(data_end,PAGE_SIZE);

    for(uint64 text = k_text_start; text < k_text_end; text += PAGE_SIZE){
        map_pages(kernel_pg_map->top_level,text,text - kernel_min + kernel_phys_min,PTE_P,text_end - text_start);
    }

    for(uint64 rodata = k_rodata_start; rodata < k_rodata_end; rodata += PAGE_SIZE){
        map_pages(kernel_pg_map->top_level,rodata,rodata - kernel_min + kernel_phys_min,PTE_P | PTE_NX, rodata_end - rodata_start);
    }



}



/*
 * Walk a page directory to find a physical address, or allocate all the way down if the alloc flag is set
 */

static pte_t* walkpgdir(p4d_t *pgdir, const uint64 *va,int alloc){

    if(!pgdir){
        return 0;
    }

    pte_t result;

    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

    pud = V2P(PTE_ADDR(pgdir[PUDX(va)]));

    if(!pud || !(*pud & PTE_P)){
        if(alloc){
             *pud = (uint64) kalloc(8) | PTE_P | PTE_RW | PTE_U;
        } else{
            return 0;
        }
    }

    pmd = V2P(PTE_ADDR(pud[PMDX(va)]));

    if(!pmd || !(*pmd & PTE_P)){
        if(alloc){
            *pmd = (uint64) kalloc(8) | PTE_P | PTE_RW | PTE_U;
        } else{
            return 0;
        }
    }

    pte = V2P(PTE_ADDR(pmd[PTX(va)]));

    if(!pte || !(*pte & PTE_P)){
        if(alloc){
            *pte = (uint64) kalloc(8) | PTE_P | PTE_RW | PTE_U;
        } else{
            return 0;
        }
    }
    return PTE_ADDR(*pte);
}

/*
 * Maps pages from VA/PA to size in page size increments.
 */
int map_pages(p4d_t *pgdir, uint64 physaddr, uint64 *va, uint32 perms,uint64 size) {

    uint64 *address, *last;
    pte_t *pte;
    address = PGROUNDDOWN((uint64)va);
    last = PGROUNDDOWN(((uint64)va) + size - 1);

    for(;;){

        if((pte = walkpgdir(pgdir, address, 1)) == 0){
            return -1;
        }

        if(*pte & PTE_P) {
            panic("remap");
        }

        *pte = physaddr | perms | PTE_P;

        if(address == last){
            break;
        }

        address += PAGE_SIZE;
        physaddr += PAGE_SIZE;
    }
    return 0;
}

struct vm_region *create_region(){

}