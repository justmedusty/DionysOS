//
// Created by dustyn on 6/18/24.
//

#ifndef DIONYSOS_IDT_H
#define DIONYSOS_IDT_H
#pragma once
#include <stdint.h>
#define KERNEL_CODE_SEGMENT_OFFSET 0x08
#define INTERRUPT_GATE 0x8e
struct idt_entry {
    unsigned short int offset_lowerbits;
    unsigned short int selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short int offset_higherbits;
};

struct interrupt_frame {
    uintptr_t ip;
    uintptr_t cs;
    uintptr_t flags;
    uintptr_t sp;
    uintptr_t ss;
};

void idt_init();
void enable_idt();
void disable_idt();
void idt_register_handler(uint8_t interrupt, unsigned long address);

#endif //DIONYSOS_IDT_H
