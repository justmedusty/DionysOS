//
// Created by dustyn on 6/19/24.
//

#ifndef KERNEL_TRAP_H
#define KERNEL_TRAP_H

void divide_by_zero();

void debug_exception();

void nmi_interrupt();

void breakpoint();

void overflow();

void bounds_check();

void illegal_opcode();

void device_not_available();

void double_fault();

void invalid_tss();

void segment_not_present();

void stack_exception();

void general_protection_fault();

void page_fault();

void floating_point_error();

void alignment_check();

void machine_check();

void simd_floating_point_error();

void trap(void);


// Processor-defined:
#define T_NONE          -1
#define T_DIVIDE         0      // divide error
#define T_DEBUG          1      // debug exception
#define T_NMI            2      // non-maskable interrupt
#define T_BRKPT          3      // breakpoint
#define T_OFLOW          4      // overflow
#define T_BOUND          5      // bounds check
#define T_ILLOP          6      // illegal opcode
#define T_DEVICE         7      // device not available
#define T_DBLFLT         8      // double fault
// #define T_COPROC      9      // reserved (not used since 486)
#define T_TSS           10      // invalid task switch segment
#define T_SEGNP         11      // segment not present
#define T_STACK         12      // stack exception
#define T_GPFLT         13      // general protection fault
#define T_PGFLT         14      // page fault
// #define T_RES        15      // reserved
#define T_FPERR         16      // floating point error
#define T_ALIGN         17      // aligment check
#define T_MCHK          18      // machine check
#define T_SIMDERR       19      // SIMD floating point error

// These are arbitrarily chosen, but with care not to overlap
// processor defined exceptions or interrupt vectors.
#define T_SYSCALL       63      // system call
#define T_DEFAULT      500      // catchall

#define T_IRQ0          32      // IRQ 0 corresponds to int T_IRQ

#define IRQ_TIMER        0
#define IRQ_KBD          1
#define IRQ_COM1         4
#define IRQ_IDE         14
#define IRQ_IDE2        15
#define IRQ_ERROR       19
#define IRQ_SPURIOUS    31

int exceptions[64] = {
        T_DIVIDE,       // 0
        T_DEBUG,        // 1
        T_NMI,          // 2
        T_BRKPT,        // 3
        T_OFLOW,        // 4
        T_BOUND,        // 5
        T_ILLOP,        // 6
        T_DEVICE,       // 7
        T_DBLFLT,       // 8
        T_NONE,         // 9
        T_TSS,          // 10
        T_SEGNP,        // 11
        T_STACK,        // 12
        T_GPFLT,        // 13
        T_PGFLT,        // 14
        T_NONE,         // 15
        T_FPERR,        // 16
        T_ALIGN,        // 17
        T_MCHK,         // 18
        T_SIMDERR,      // 19
        T_NONE,         // 20
        T_NONE,         // 21
        T_NONE,         // 22
        T_NONE,         // 23
        T_NONE,         // 24
        T_NONE,         // 25
        T_NONE,         // 26
        T_NONE,         // 27
        T_NONE,         // 28
        T_NONE,         // 29
        T_NONE,         // 30
        T_NONE,         // 31
        IRQ_TIMER,      // 32
        IRQ_KBD,        // 33
        T_NONE,         // 34
        T_NONE,         // 35
        IRQ_COM1,       // 36
        T_NONE,         // 37
        T_NONE,         // 38
        T_NONE,         // 39
        T_NONE,         // 40
        T_NONE,         // 41
        T_NONE,         // 42
        T_NONE,         // 43
        T_NONE,         // 44
        T_NONE,         // 45
        T_NONE,         // 46
        T_NONE,         // 47
        T_NONE,         // 48
        T_NONE,         // 49
        T_NONE,         // 50
        T_NONE,         // 51
        T_NONE,         // 52
        T_NONE,         // 53
        T_NONE,         // 54
        T_NONE,         // 55
        T_NONE,         // 56
        T_NONE,         // 57
        T_NONE,         // 58
        T_NONE,         // 59
        T_NONE,         // 60
        T_NONE,         // 61
        T_NONE,         // 62
        T_SYSCALL      //63
};
#endif //KERNEL_TRAP_H
