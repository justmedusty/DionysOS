//
// Created by dustyn on 6/24/24.
//

#pragma once

#include "include/memory/pmm.h"
#include "include/definitions/types.h"

#define Phys2Virt(addr) (void *)(((uint64_t)addr) + (uint64_t)hhdm_offset)
#define Virt2Phys(addr) (void *)(((uint64_t)addr) - (uint64_t)hhdm_offset)

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
#define PGADDR(p4d, pud, pmd, pte, offset) ((uint64_t)((p4d << P4DXSHIFT | pud << PUDXSHIFT | (pmd) << PMDDXSHIFT | (pte) << PTXSHIFT | (offset)))


#define PTXSHIFT        12UL      // offset of PTX in a linear address
#define PMDXSHIFT       21UL     // offset of PMDX in a linear address
#define PUDXSHIFT       30UL     // offset of PUDX in a linear address
#define P4DXSHIFT       39UL     // offset of P4DX

#define PGROUNDUP(sz)  (uint64_t) (uint64_t)((((uint64_t)sz)+PAGE_SIZE-1) & ~(uint64_t)(PAGE_SIZE-1))
#define PGROUNDDOWN(a) (uint64_t) ((((uint64_t)a)) & ~(uint64_t)(PAGE_SIZE-1))

// Page table/directory entry flags.
#define PTE_P           0x001ULL   // Present
#define PTE_RW          0x2ULL
#define PTE_U           0x004ULL   // User
#define PTE_A           0x020ULL  //accessed , for demand paging
#define PTE_PS          0x080ULL   // Page Size
#define PTE_NX          1ULL << 63ULL// no execute
#define PTE_PCD         0x010ULL // page cache disable
#define PTE_PWT         0x008ULL //page write through


#define PTE_PAT         1UL << 7UL //Only bit 7 in your run-of-the-mill 4k PTEs

#define PAT_MSR 0x277
#define PAT_UC 0x00 // uncachable
#define PAT_WC 0x01 //write combining
#define PAT_WT 0x04 //write through
#define PAT_WP 0x05 //write protect
#define PAT_WB 0x06 //write back
#define PAT_UC_MINUS 0x07 // uncached-

// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uint64_t)(pte) & ~0xFFFULL)
#define PTE_FLAGS(pte)  ((uint64_t)(pte) &  0xFFFULL)

#endif