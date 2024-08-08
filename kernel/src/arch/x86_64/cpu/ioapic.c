//
// Created by dustyn on 6/21/24.
//
#include "ioapic.h"

#include <include/arch_paging.h>

void write_ioapic(madt_ioapic *ioapic, uint8 reg, uint32 value){
    *((volatile uint32*) P2V(ioapic->apic_addr) + IOAPIC_REG_SELECT) = reg;
    *((volatile uint32*) P2V(ioapic->apic_addr) + IOAPIC_IOWIN) = value;
}

uint32 read_ioapic(madt_ioapic *ioapic, uint8 reg){
    *((volatile uint32*) P2V(ioapic->apic_addr) + IOAPIC_REG_SELECT) = reg;
    return *((volatile uint32*) P2V(ioapic->apic_addr) + IOAPIC_IOWIN);
}

void ioapic_set_entry(madt_ioapic *ioapic, uint8 index, uint64 data){

}

void ioapic_redirect_irq(uint32 lapic_id,uint8 vector,uint8 irq,uint8 mask){

}

uint32 ioapic_get_redirect_irq(uint32 lapic_id,uint8 vector,uint8 irq,uint8 mask){

}

uint64 ioapic_init(){

}