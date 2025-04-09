//
// Created by dustyn on 6/16/24.
//
#pragma once
#include <include/definitions/definitions.h>
#ifdef __x86_64__
#include "immintrin.h"
#include <include/drivers/serial/uart.h>

#include "include/definitions/types.h"
// Routines to let C code use special x86 instructions.


// Reads a byte from the specified I/O port.
static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    asm volatile("in %1,%0" : "=a" (data) : "d" (port));
    return data;
}

// Reads a sequence of 32-bit values from the specified I/O port into memory.
static inline void insl(int port, void* addr, int cnt) {
    asm volatile("cld; rep insl" :
        "=D" (addr), "=c" (cnt) :
        "d" (port), "0" (addr), "1" (cnt) :
        "memory", "cc");
}

// Reads a sequence of 64-bit values from the specified I/O port into memory.
static inline void insq(int port, void* addr, int cnt) {
    asm volatile("cld; rep insq" :
        "=D" (addr), "=c" (cnt) :
        "d" (port), "0" (addr), "1" (cnt) :
        "memory", "cc");
}

// Writes a byte to the specified I/O port.
static inline void outb(uint16_t port, uint8_t data) {
    asm volatile("out %0,%1" : : "a" (data), "d" (port));
}

// Writes a 16-bit value to the specified I/O port.
static inline void outw(uint16_t port, uint16_t data) {
    asm volatile("out %0,%1" : : "a" (data), "d" (port));
}

// Writes a sequence of 32-bit values from memory to the specified I/O port.
static inline void outsl(int port, const void* addr, int cnt) {
    asm volatile("cld; rep outsl" :
        "=S" (addr), "=c" (cnt) :
        "d" (port), "0" (addr), "1" (cnt) :
        "cc");
}

// Writes a sequence of 64-bit values from memory to the specified I/O port.
static inline void outsq(int port, const void* addr, int cnt) {
    asm volatile("cld; rep outsq" :
        "=S" (addr), "=c" (cnt) :
        "d" (port), "0" (addr), "1" (cnt) :
        "cc");
}

// Writes a byte value repeatedly to a memory location.
static inline void stosb(void* addr, int data, int cnt) {
    asm volatile("cld; rep stosb" :
        "=D" (addr), "=c" (cnt) :
        "0" (addr), "1" (cnt), "a" (data) :
        "memory", "cc");
}

// Writes a 32-bit value repeatedly to a memory location.
static inline void stosl(void* addr, int data, int cnt) {
    asm volatile("cld; rep stosl" :
        "=D" (addr), "=c" (cnt) :
        "0" (addr), "1" (cnt), "a" (data) :
        "memory", "cc");
}

// Writes a 64-bit value repeatedly to a memory location.
static inline void stosq(void* addr, long long data, int cnt) {
    asm volatile("cld; rep stosq" :
        "=D" (addr), "=c" (cnt) :
        "0" (addr), "1" (cnt), "a" (data) :
        "memory", "cc");
}


// Loads the Task Register (TR).
static inline void ltr(uint16_t sel) {
    asm volatile("ltr %0" : : "r" (sel));
}

// Reads the EFLAGS register.
static inline uint64_t readeflags(void) {
    uint32_t eflags;
    asm volatile("pushfl; popq %0" : "=r" (eflags));
    return eflags;
}

// Loads a 16-bit value into the GS segment register.
static inline void loadgs(uint16_t v) {
    asm volatile("movw %0, %%gs" : : "r" (v));
}

// Disables trap.
static inline void cli(void) {
    asm volatile("cli");
}

// Enables trap.
static inline void sti(void) {
    asm volatile("sti");
}

// Atomic exchange of a value.
static inline uint64_t xchg(volatile uint64_t *addr, uint64_t newval) {
    uint64_t result;
    asm volatile("lock; xchg %0, %1" :
        "+m" (*addr), "=a" (result) :
        "1" (newval) :
        "cc");
    return result;
}

static inline uint64_t rcr2(void) {
    uint64_t val;
    asm volatile("mov %%cr2, %0" : "=r" (val));
    return val;
}

// Loads a value into the CR3 register.
static inline void lcr3(uint64_t val) {
    asm volatile("movq %0,%%cr3" : : "r" (val));
}

// reads a value from the CR3 register.
static inline uint64_t rcr3(void) {
    uint64_t destination;
    asm volatile("mov %%cr3,%0" : "=r"(destination));
    return destination;
}


static inline void flush_tlb() {
    asm volatile("mov %%cr3, %%rax\n\t"
        "mov %%rax, %%cr3\n\t"
        :
        :
        : "rax");
}

static inline uint64_t interrupts_enabled() {
    uint64_t flags;
    asm volatile(
          "pushfq\n\t"       // Push RFLAGS onto the stack
          "popq %0\n\t"      // Pop the value from the stack into the variable
          : "=r" (flags)     // Output operand (RFLAGS value stored in 'flags')
          :                  // No input operands
          : "memory"         // Memory is modified
      );
    return ((flags & BIT(9)) > 0);
}

static inline void dflush64(void *ptr) {
    __asm__ volatile("clflush (%0)" :: "r"(ptr));
}

//PAGEBREAK: 36
// Layout of the trap frame built on the stack by the
// hardware and by trapasm.S, and passed to trap().
struct trapframe {
    // registers as pushed by pusha
    unsigned long long rdi;
    unsigned long long rsi;
    unsigned long long rbp;
    unsigned long long resp; // useless & ignored
    unsigned long long rbx;
    unsigned long long rdx;
    unsigned long long rcx;
    unsigned long long rax;

    // rest of trap frame
    uint16_t gs;
    uint16_t padding1;
    uint16_t fs;
    uint16_t padding2;
    uint16_t es;
    uint16_t padding3;
    uint16_t ds;
    uint16_t padding4;
    uint32_t trapno;

    // below here defined by x86 hardware
    unsigned long long err;
    unsigned long long rip;
    uint16_t cs;
    uint16_t padding5;
    unsigned long long eflags;

    // below here only when crossing rings, such as from user to kernel
    uint32_t rsp;
    uint16_t ss;
    uint16_t padding6;
};
#endif