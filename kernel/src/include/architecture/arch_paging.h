//
// Created by dustyn on 6/24/24.
//

#pragma once
#include "include/memory/pmm.h"
#include "include/definitions/types.h"
#define P2V(addr) (void *)(((uint64_t)addr) + (uint64_t)hhdm_offset)
#define V2P(addr) (void *)(((uint64_t)addr) - (uint64_t)hhdm_offset)

void arch_paging_init();
#ifdef __x86_64__
// A virtual address has  5 parts, 6 if we were using 5 level paging structure as follows:
//
// +-------9--------+----------9--------+---------9---------+---------9---------+---------12---------+
// |   P4D  Table   |     PUD Table     |      PMD Table    |      PTE Table    |   Offset within    |
// |      Index     |     Index         |        Index      |        Index      |         page       |
// +----------------+-------------------+-------------------+-------------------+--------------------+
//

// Page directory and page table constants.
#define NP4DENTRIES     512    // # directory entries per page middle directory
#define NPUDENTRIES     512    // # directory entries per page upper directory
#define NPMDENTRIES     512    // # directory entries per page middle directory
#define NPTENTRIES      512    // # PTEs per page table

#define PAGE_DIR_MASK 0x1FFUL
#define PAGE_OFFSET_MASK 0x3FF
#define PTE_ADDRESS_MASK ~0xFFF

extern p4d_t *global_pg_dir;

// page 4 directory index
#define P4DX(va)         (((uint64_t)(va) >> P4DXSHIFT) & PAGE_DIR_MASK)
// page upper directory index
#define PUDX(va)         (((uint64_t)(va) >> PUDXSHIFT) & PAGE_DIR_MASK)

// page middle directory index
#define PMDX(va)         (((uint64_t)(va) >> PMDXSHIFT) & PAGE_DIR_MASK)

// page table index
#define PTX(va)         (((uint64_t)(va) >> PTXSHIFT) & PAGE_DIR_MASK)

// construct virtual address from indexes (long mode) and offset
#define PGADDR(p4d,pud,pmd, pte, offset) ((uint64_t)((p4d << P4DXSHIFT | pud << PUDXSHIFT | (pmd) << PMDDXSHIFT | (pte) << PTXSHIFT | (offset)))



#define PTXSHIFT        12UL      // offset of PTX in a linear address
#define PMDXSHIFT       21UL     // offset of PMDX in a linear address
#define PUDXSHIFT       30UL     // offset of PUDX in a linear address
#define P4DXSHIFT       39UL     // offset of P4DX

#define PGROUNDUP(sz)  (uint64_t) (uint64_t)((((uint64_t)sz)+PAGE_SIZE-1) & ~(uint64_t)(PAGE_SIZE-1))
#define PGROUNDDOWN(a) (uint64_t) ((((uint64_t)a)) & ~(uint64_t)(PAGE_SIZE-1))

// Page table/directory entry flags.
#define PTE_P           (uint64_t)0x001UL   // Present
#define PTE_RW          (uint64_t)0x2UL
#define PTE_U           (uint64_t)0x004UL   // User
#define PTE_A           (uint64_t)0x020UL  //accessed , for demand paging
#define PTE_PS          (uint64_t)0x080UL   // Page Size
#define PTE_NX          (uint64_t) (1UL << 63UL) // no execute


// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uint64_t)(pte) & ~0xFFFUL)
#define PTE_FLAGS(pte)  ((uint64_t)(pte) &  0xFFFUL)

#endif