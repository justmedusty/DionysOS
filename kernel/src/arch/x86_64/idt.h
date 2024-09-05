//
// Created by dustyn on 6/18/24.
//


#pragma once

#include "gdt.h"
#include "include/types.h"
#include <stdint.h>

struct idtr_desc {
    uint16_t sz;
    uint64_t off;
} __attribute__((packed));

// Gate descriptor describes an ISR.
struct gate_desc {
    uint16_t off_1;
    struct segment_selector segment_selector;
    uint8_t ist : 3;
    uint8_t : 5;
    enum system_segment_type gate_type : 4;
    uint8_t : 1;
    uint8_t dpl : 2;
    uint8_t p : 1;
    uint16_t off_2;
    uint32_t off_3;
    uint32_t : 32;
} __attribute__((packed));

// Read the IDT. See https://wiki.osdev.org/IDT
inline void read_idt(struct idtr_desc *idtr) {
    __asm__ volatile("sidt %0" : "=m"(*idtr));
}

inline void load_idtr(struct idtr_desc *idtr) {
    __asm__ volatile("lidt %0" : "=m"(*idtr));
}


void idt_init(void);
void irq_register(uint8 vec, void* handler);
void irq_unregister(uint8 vec);
uint8 idt_get_vector();
void idt_reload();
void irq_handler(uint8 vec);
void irq_handler_init();

