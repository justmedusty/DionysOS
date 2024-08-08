//
// Created by dustyn on 8/8/24.
//

#ifndef IOAPIC_H
#define IOAPIC_H
#include "include/madt.h"
#include "include/acpi.h"

#define IOAPIC_ID 0x0
#define IOAPIC_VERSION 0x1
#define IOAPIC_ARB 0x02
#define IOAPIC_REDTBL 0x10

#define IOAPIC_REG_SELECT 0x0
#define IOAPIC_IOWIN 0x10


void write_ioapic(madt_ioapic *ioapic, uint8 register, uint32 value);
uint32 read_ioapic(madt_ioapic *ioapic, uint8 register);
void ioapic_set_entry(madt_ioapic *ioapic, uint8 index, uint64 data);
void ioapic_redirect_irq(uint32 lapic_id,uint8 vector,uint8 irq,uint8 mask);
uint32 ioapic_get_redirect_irq(uint32 lapic_id,uint8 vector,uint8 irq,uint8 mask);
uint64 ioapic_init();
#endif //IOAPIC_H
