//
// Created by dustyn on 6/25/24.
//
#include "include/types.h"
#include "include/arch_paging.h"
#include "include/pmm.h"


p4d_t global_pg_dir[NP4DENTRIES];
pud_t upper_pg_dir[NPUDENTRIES];
pmd_t middle_pg_dir[NPMDENTRIES];
pte_t page_table_entries[NPTENTRIES];



static pte_t* walkpgdir(p4d_t *pgdir, const uint64 *va,int alloc){

    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

    p4d = &pgdir[P4DX(va)];
    pud = &p4d[PUDX(va)];
    pmd = &pud[PMDX(va)];
    pte = &pmd[PTX(va)];
}

/*
 * Boiler plate right now will change obviously
 */
void *get_phys_addr(uint64 *virtualaddr) {

    // Here you need to check whether the PD entry is present.
    pte_t *pte = &
    // Here you need to check whether the PT entry is present.
    //return (void *)((pt[ptindex] & ~0xFFF) + ((unsigned long)virtualaddr & 0xFFF));
}

/*
 * Boiler plate right now will change obviously
 */
void map_page(void *physaddr, void *virtualaddr, unsigned int flags) {
    // Make sure that both addresses are page-aligned.

    unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

    unsigned long *pd = (unsigned long *)0xFFFFF000;
    // Here you need to check whether the PD entry is present.
    // When it is not present, you need to create a new empty PT and
    // adjust the PDE accordingly.

    unsigned long *pt = ((unsigned long *)0xFFC00000) + (0x400 * pdindex);
    // Here you need to check whether the PT entry is present.
    // When it is, then there is already a mapping present. What do you do now?

    pt[ptindex] = ((unsigned long)physaddr) | (flags & 0xFFF) | 0x01; // Present

    // Now you need to flush the entry in the TLB
    // or you might not notice the change.
}
