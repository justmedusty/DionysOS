//
// Created by dustyn on 8/8/24.
//

#ifndef IOAPIC_H
#define IOAPIC_H
#include "x86_64/madt.h"
#include "x86_64/acpi.h"

#define IOAPIC_ID 0x0
#define IOAPIC_VERSION 0x1
#define IOAPIC_ARB 0x02
#define IOAPIC_REDTBL 0x10
#define IOAPIC_REG_SELECT 0x0
#define IOAPIC_IOWIN 0x10

void pic_disable();
void write_ioapic(uint32_t ioapic, uint32_t reg, uint32_t value);
uint32_t read_ioapic(uint32_t ioapic, uint32_t reg);
void ioapic_set_entry(uint32_t ioapic, uint8_t index, uint64_t data);
void ioapic_redirect_irq(uint32_t lapic_id,uint8_t vector,uint8_t irq,uint8_t mask);
void ioapic_redirect_gsi(uint32_t lapic_id,uint8_t vector,uint32_t gsi,uint16_t flags,uint8_t mask);
uint32_t ioapic_get_gsi(uint32_t gsi);
void ioapic_init();
#endif //IOAPIC_H
