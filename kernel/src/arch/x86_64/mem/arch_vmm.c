//
// Created by dustyn on 6/25/24.
//

/*
 * Taking inspo from xv6 to keep things as readable and simple as possible here.
 */

#include "include/types.h"
#include "include/arch_paging.h"
#include "include/pmm.h"
#include "include/kalloc.h"
#include "include/x86.h"
#include "include/mem_bounds.h"
#include "include/uart.h"
#include "include/cpu.h"
#include "include/arch_vmm.h"

p4d_t *global_pg_dir = 0;
p4d_t kernel_pg_dir;

void switch_page_table(p4d_t *page_dir){
    lcr3(page_dir);
}

void arch_init_vmm(){

    uint64 page_directory_physical = 0;
    asm volatile("movq %%cr3,%0" : "=r"(page_directory_physical));
    if (!page_directory_physical) {
        panic("No page directory!");
    }
    uint64 page_directory_virtual = page_directory_physical + hhdm_offset;
    global_pg_dir = (uint64 *)page_directory_virtual;
    serial_printf("VMM initialized.\nPhysical Page Directory Located at %x.64\n",page_directory_physical);
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

    pud = PTE_ADDR(pgdir[PUDX(va)]) + hhdm_offset;

    if(!pud || !(*pud & PTE_P)){
        if(alloc){
             *pud = (uint64) kalloc(8) | PTE_P | PTE_RW | PTE_U;
        } else{
            return 0;
        }
    }

    pmd = PTE_ADDR(pud[PMDX(va)] + hhdm_offset);

    if(!pmd || !(*pmd & PTE_P)){
        if(alloc){
            *pmd = (uint64) kalloc(8) | PTE_P | PTE_RW | PTE_U;
        } else{
            return 0;
        }
    }

    pte = PTE_ADDR(pmd[PTX(va)] + hhdm_offset);

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
