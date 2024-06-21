//
// Created by dustyn on 6/16/24.
//

#ifndef DIONYSOS_PAGING_H
#define DIONYSOS_PAGING_H

// A virtual address has a three-part structure as follows:
//
// +-------9--------+----------9--------+---------9---------+---------9---------+---------12---------+
// |   P4D  Table   |     PUD Table     |      PMD Table    |      PTE Table    |   Offset within    |
// |      Index     |     Index         |        Index      |        Index      |         page       |
// +----------------+-------------------+-------------------+-------------------+--------------------+
//


// page 4 directory index
#define P4DX(va)         (((uint64)(va) >> P4DXSHIFT) & 0x1FF)
// page upper directory index
#define PUDX(va)         (((uint64)(va) >> PUDXSHIFT) & 0x1FF)

// page middle directory index
#define PMDX(va)         (((uint64)(va) >> PMDXSHIFT) & 0x1FF)

// page table index
#define PTX(va)         (((uint64)(va) >> PTXSHIFT) & 0x1FF)

// construct virtual address from indexes (long mode) and offset
#define PGADDR(p4d,pud,pmd, pte, offset) ((uint64)((p4d << P4DXSHIFT | pud << PUDXSHIFT | (pmd) << PMDDXSHIFT | (pte) << PTXSHIFT | (offset)))

// Page directory and page table constants.
#define NP4DENTRIES     512    // # directory entries per page middle directory
#define NPUDENTRIES     512    // # directory entries per page upper directory
#define NPMDENTRIES     512    // # directory entries per page middle directory
#define NPTENTRIES      512    // # PTEs per page table
#define PAGESIZE          4096   // bytes mapped by a page

#define PTXSHIFT        12      // offset of PTX in a linear address
#define PMDXSHIFT       21     // offset of PMDX in a linear address
#define PUDXSHIFT       30     // offset of PUDX in a linear address
#define P4DXSHIFT       39     // offset of P4DX

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

// Page table/directory entry flags.
#define PTE_P           0x001   // Present
#define PTE_W           0x002   // Writeable
#define PTE_U           0x004   // User
#define PTE_A           0x020   //accessed , for demand paging
#define PTE_PS          0x080   // Page Size

// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uint64)(pte) & ~0xFFF)
#define PTE_FLAGS(pte)  ((uint64)(pte) &  0xFFF)
#endif //DIONYSOS_PAGING_H
