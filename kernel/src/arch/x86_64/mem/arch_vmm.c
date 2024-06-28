//
// Created by dustyn on 6/25/24.
//
#include "include/types.h"
#include "include/arch_paging.h"
#include "include/pmm.h"
#include "include/x86.h"

p4d_t global_pg_dir[NP4DENTRIES];

static pte_t* walkpgdir(p4d_t *pgdir, const uint64 *va,int alloc){

    if(!pgdir){
        return 0;
    }

    pte_t result;

    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

    pud = &pgdir[PUDX(va)] + hhdm_offset;

    if(!pud || !(*pud & PTE_P)){
        return 0;
    }

    pmd = PTE_ADDR(pud[PMDX(va)] + hhdm_offset);

    if(!pmd || !(*pmd & PTE_P)){
        return 0;
    }

    pte = PTE_ADDR(pmd[PTX(va)] + hhdm_offset);

    if(pte && (*pte & PTE_P)){

       result = PTE_ADDR(*pte);

    } else{

        result = 0;

    }
    return result;
}

void *get_phys_addr(uint64 *va) {

    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    p4d = &global_pg_dir[P4DX(va)];
    pud = &p4d[PUDX(va)];
    pmd = &pud[PMDX(va)];
    pte = &pmd[PTX(va)];
    if(*pte & PTE_P){
        return (uint64 *)((*pte & PTE_ADDRESS_MASK) + ((uint64)va & PAGE_OFFSET_MASK));
    } else{
        return 0;
    }
}

/*
 * Boiler plate right now will change obviously
 */
void map_pages(p4d_t *page_dir, uint64 *physaddr, uint64 *va, unsigned int flags,uint32 size) {
    // Make sure that both addresses are page-aligned.

    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

    pud = &page_dir[PUDX(va)];
    pmd = &pud[PMDX(va)];
    pte = &pmd[PTX(va)];





   // pt[ptindex] = ((unsigned long)physaddr) | (flags & 0xFFF) | 0x01; // Present

    // Now you need to flush the entry in the TLB
    // or you might not notice the change.
}
