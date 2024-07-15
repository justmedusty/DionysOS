//
// Created by dustyn on 6/24/24.
//

#ifndef KERNEL_ARCH_PAGING_H
#define KERNEL_ARCH_PAGING_H

#define P2V(addr) ((void *)((uint64)addr) + hhdm_offset)
#define V2P(addr) ((void *)((uint64)addr) - hhdm_offset)

void arch_paging_init();

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
#define PAGE_SIZE       4096   // bytes mapped by a page

#define PAGE_DIR_MASK 0x1FF
#define PAGE_OFFSET_MASK 0x3FF
#define PTE_ADDRESS_MASK ~0xFFF

extern p4d_t *global_pg_dir;

// page 4 directory index
#define P4DX(va)         (((uint64)(va) >> P4DXSHIFT) & PAGE_DIR_MASK)
// page upper directory index
#define PUDX(va)         (((uint64)(va) >> PUDXSHIFT) & PAGE_DIR_MASK)

// page middle directory index
#define PMDX(va)         (((uint64)(va) >> PMDXSHIFT) & PAGE_DIR_MASK)

// page table index
#define PTX(va)         (((uint64)(va) >> PTXSHIFT) & PAGE_DIR_MASK)

// construct virtual address from indexes (long mode) and offset
#define PGADDR(p4d,pud,pmd, pte, offset) ((uint64)((p4d << P4DXSHIFT | pud << PUDXSHIFT | (pmd) << PMDDXSHIFT | (pte) << PTXSHIFT | (offset)))



#define PTXSHIFT        12      // offset of PTX in a linear address
#define PMDXSHIFT       21     // offset of PMDX in a linear address
#define PUDXSHIFT       30     // offset of PUDX in a linear address
#define P4DXSHIFT       39     // offset of P4DX

#define PGROUNDUP(sz)  (((sz)+PAGE_SIZE-1) & ~(PAGE_SIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PAGE_SIZE-1))

// Page table/directory entry flags.
#define PTE_P           0x001   // Present
#define PTE_RW          0x2
#define PTE_U           0x004   // User
#define PTE_A           0x020   //accessed , for demand paging
#define PTE_PS          0x080   // Page Size
#define PTE_NX          1 << 63 // no execute


// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uint64)(pte) & ~0xFFF)
#define PTE_FLAGS(pte)  ((uint64)(pte) &  0xFFF)
#endif //KERNEL_ARCH_PAGING_H
