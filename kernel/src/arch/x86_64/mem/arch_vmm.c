//
// Created by dustyn on 6/25/24.
//

/*
 * Taking inspo from xv6 to keep things as readable and simple as possible here.
 */

#include "include/types.h"
#include "include/arch_paging.h"
#include "include/pmm.h"
#include "include/kheap.h"
#include "include/x86.h"
#include "include/mem_bounds.h"
#include "include/uart.h"

p4d_t global_pg_dir[NP4DENTRIES];
p4d_t kernel_pg_dir;
/*
static struct dev_map {
    void *virt;
    uint64 phys_start;
    uint64 phys_end;
    int32 perm;
} dev_map[] = {
        { (void*)kernel_min,          kernel_phys_min,      EXTMEM,    PTE_W}, // I/O space
        { (void*)DEVSPACE,kernel_phys_max - DEVSPACE, kernel_phys_max, PTE_W}, // more devices
};
 */

void switch_page_table(p4d_t *page_dir){
    lcr3(page_dir);
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
void map_pages(p4d_t *pgdir, uint64 physaddr, uint64 *va, uint32 perms,uint64 size) {

    uint64 *address, *last;
    pte_t *pte;
    address = PGROUNDDOWN((uint64)va);
    last = PGROUNDDOWN(((uint64)va) + size - 1);

    for(;;){

        if((pte = walkpgdir(pgdir, address, 1)) == 0){
            return -1;
        }

        if(*pte & PTE_P) {
            bootstrap_panic("remap");
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
