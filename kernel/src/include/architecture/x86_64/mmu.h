//
// Created by dustyn on 6/16/24.
//

#ifndef DIONYSOS_MMU_H
#define DIONYSOS_MMU_H
#include <include/types.h>
// This file contains definitions for the
// x86 memory management unit (MMU).
// Eflags register
#define FL_IF           0x00000200      // Interrupt Enable

// Control Register flags
#define CR0_PE          0x00000001      // Protection Enable
#define CR0_WP          0x00010000      // Write Protect
#define CR0_PG          0x80000000      // Paging
#define CR0_PG_OFF     ~0x80000000      //turn paging off
#define CR4_PAE
#define CR4_PSE         0x00000010      // Page size extension

// various segment selectors.
#define SEG_KCODE 1  // kernel code
#define SEG_KDATA 2  // kernel types+stack
#define SEG_UCODE 3  // user code
#define SEG_UDATA 4  // user types+stack
#define SEG_TSS   5  // this process's task state

// cpu->gdt[NSEGS] holds the above segments.
#define NSEGS     6

#ifndef __ASSEMBLER__
// Segment Descriptor for 64-bit mode
struct segdesc {
    uint64_t lim_15_0 : 16;  // Low bits of segment limit
    uint64_t base_15_0 : 16; // Low bits of segment base address
    uint64_t base_23_16 : 8; // Middle bits of segment base address
    uint64_t type : 4;       // Segment type (see STS_ constants)
    uint64_t s : 1;          // 0 = system, 1 = application
    uint64_t dpl : 2;        // Descriptor Privilege Level
    uint64_t p : 1;          // Present
    uint64_t lim_19_16 : 4;  // High bits of segment limit
    uint64_t avl : 1;        // Unused (available for software use)
    uint64_t l : 1;          // 64-bit code segment (IA-64e mode only)
    uint64_t db : 1;         // 0 = 16-bit segment, 1 = 64-bit segment (must be 0 for 64-bit)
    uint64_t g : 1;          // Granularity: limit scaled by 4K when set
    uint64_t base_31_24 : 8; // High bits of segment base address
    uint64_t base_63_64 : 64; // Upper bits of segment base address
    uint64_t reserved : 64; // Reserved
};

// Normal segment for 64-bit mode
#define SEG(type, base, lim, dpl) (struct segdesc)  \
{ ((lim) >> 12) & 0xffff, (uint64_t)(base) & 0xffff,    \
  ((uint64_t)(base) >> 16) & 0xff, type, 1, dpl, 1,     \
  (uint64_t)(lim) >> 28, 0, 1, 0, 1, (uint64_t)(base) >> 24, \
  (uint64_t)(base) >> 64, 0 }
#define SEG16(type, base, lim, dpl) (struct segdesc) \
{ (lim) & 0xffff, (uint64_t)(base) & 0xffff,            \
  ((uint64_t)(base) >> 16) & 0xff, type, 1, dpl, 1,     \
  (uint64_t)(lim) >> 16, 0, 0, 1, 0, (uint64_t)(base) >> 24, \
  (uint64_t)(base) >> 64, 0 }
#endif

#define DPL_USER    0x3     // User DPL

// Application segment type bits
#define STA_X       0x8     // Executable segment
#define STA_W       0x2     // Writeable (non-executable segments)
#define STA_R       0x2     // Readable (executable segments)

// System segment type bits
#define STS_T64A    0x9     // Available 32-bit TSS
#define STS_IG64    0xE     // 32-bit Interrupt Gate
#define STS_TG64    0xF     // 32-bit Trap Gate

// A virtual address 'la' has a three-part structure as follows:
//
// +-------9--------+----------9--------+---------9---------+---------9---------+---------12---------+
// |   P4D  Table   |     PUD Table     |      PMD Table    |      PTE Table    |   Offset within    |
// |      Index     |     Index         |        Index      |        Index      |         page       |
// +----------------+-------------------+-------------------+-------------------+--------------------+
//


// page 4 directory index
#define P4DX(va)         (((uint64_t)(va) >> P4DXSHIFT) & 0x1FF)
// page upper directory index
#define PUDX(va)         (((uint64_t)(va) >> PUDXSHIFT) & 0x1FF)

// page middle directory index
#define PMDX(va)         (((uint64_t)(va) >> PMDXSHIFT) & 0x1FF)

// page table index
#define PTX(va)         (((uint64_t)(va) >> PTXSHIFT) & 0x1FF)

// construct virtual address from indexes (long mode) and offset
#define PGADDR(p4d,pud,pmd, pte, offset) ((uint64_t)((p4d << P4DXSHIFT | pud << PUDXSHIFT | (pmd) << PMDDXSHIFT | (pte) << PTXSHIFT | (offset)))

// Page directory and page table constants.
#define NP4DENTRIES     512    // # directory entries per page middle directory
#define NPUDENTRIES     512    // # directory entries per page upper directory
#define NPMDENTRIES     512    // # directory entries per page middle directory
#define NPTENTRIES      512    // # PTEs per page table
#define PGSIZE          4096   // bytes mapped by a page

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
#define PTE_ADDR(pte)   ((uint64_t)(pte) & ~0xFFF)
#define PTE_FLAGS(pte)  ((uint64_t)(pte) &  0xFFF)

#ifndef __ASSEMBLER__
typedef uint64_t pte_t;


// Set up a normal interrupt/trap gate descriptor.
// - istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate.
//   interrupt gate clears FL_IF, trap gate leaves FL_IF alone
// - sel: Code segment selector for interrupt/trap handler
// - off: Offset in code segment for interrupt/trap handler
// - dpl: Descriptor Privilege Level -
//        the privilege level required for software to invoke
//        this interrupt/trap gate explicitly using an int instruction.
#define SETGATE(gate, istrap, sel, off, d)                \
{                                                         \
  (gate).off_15_0 = (uint64_t)(off) & 0xffff;                \
  (gate).cs = (sel);                                      \
  (gate).args = 0;                                        \
  (gate).rsv1 = 0;                                        \
  (gate).type = (istrap) ? STS : STS_IG64;           \
  (gate).s = 0;                                           \
  (gate).dpl = (d);                                       \
  (gate).p = 1;                                           \
  (gate).off_31_16 = (uint64_t)(off) >> 16;                  \
}

#endif
#endif //DIONYSOS_MMU_H
