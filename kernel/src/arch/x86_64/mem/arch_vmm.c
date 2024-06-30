//
// Created by dustyn on 6/25/24.
//
#include "include/types.h"
#include "include/arch_paging.h"
#include "include/pmm.h"
#include "include/kheap.h"
#include "include/x86.h"
#include "include/mem_bounds.h"

p4d_t global_pg_dir[NP4DENTRIES];
p4d_t kernel_pg_dir;

/*
 * Walk a page directory to find a physical address, or allocate all the way down if the alloc flag is set
 */

static struct kmap {
    void *virt;
    uint64 phys_start;
    uint64 phys_end;
    int32 perm;
    //This needs to change
} kmap[] = {
        { (void*)KERNBASE, 0,             EXTMEM,    PTE_W}, // I/O space
        { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},     // kern text+rodata
        { (void*)data,     V2P(data),     PHYSTOP,   PTE_W}, // kern data+memory
        { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W}, // more devices
};
//This will set up the kernel page dir and load the CR3 register
void kernel_vm_setup(){

}
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

void map_pages(p4d_t *pgdir, uint64 *physaddr, uint64 *va, uint32 flags,uint32 size) {
    // Make sure that both addresses are page-aligned.

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






    // pt[ptindex] = ((unsigned long)physaddr) | (flags & 0xFFF) | 0x01; // Present

    // Now you need to flush the entry in the TLB
    // or you might not notice the change.
}


//this will switch to the kernel page dir
void kvm_switch(){

}