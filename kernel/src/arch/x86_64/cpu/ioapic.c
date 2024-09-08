//
// Created by dustyn on 6/21/24.
//
#include <include/arch/arch_cpu.h>
#include "include/arch/arch_global_interrupt_controller.h"
#include <include/arch/arch_paging.h>
#include "include/arch/x86_64/asm_functions.h"
#include <include/uart.h>

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_DATA PIC1 + 1
#define PIC2_DATA PIC2 + 1

void ioapic_init() {
    uint32 val = read_ioapic(0, IOAPIC_VERSION);
    int32 count = ((val >> 16) & 0xFF);


    for (uint8 i = 0; i <= count; ++i) {
        write_ioapic(0, IOAPIC_REDTBL + 2 * i, 0x00010000 | (32 + i));
        write_ioapic(0, IOAPIC_REDTBL + 2 * i + 1, 0); // redir cpu
    }
}

void pic_disable() {
    //Mask all legacy PIC interrupts
    ioapic_init();
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}


void write_ioapic(uint32 ioapic, uint32 reg, uint32 value) {
    uint64 base = (uint64)P2V(madt_ioapic_list[ioapic]->apic_addr);
    *((volatile uint32*)base) = reg;
    base += IOAPIC_IOWIN;
    *((uint32*)base) = value;
}

uint32 read_ioapic(uint32 ioapic, uint32 reg) {
    uint64 base = (uint64)P2V(madt_ioapic_list[ioapic]->apic_addr);
    *(uint32*)base = reg;
    base += IOAPIC_IOWIN;
    return *((volatile uint32*)base);
}

uint64 ioapic_gsi_count(uint32 ioapic) {
    if (ioapic > madt_ioapic_len) {
        return 0;
    }
    uint64 value = (read_ioapic(ioapic, 1) & 0xFF0000) >> 16;
    return value;
}

uint32 ioapic_get_gsi(uint32 gsi) {
    for (uint64 i = 0; i < madt_ioapic_len; i++) {
        if (madt_ioapic_list[i]->gsi_base <= gsi && madt_ioapic_list[i]->gsi_base + ioapic_gsi_count(i) > gsi) {
            return i;
        }
    }
    panic("Cannot determine IOAPIC from GSI\n");
}

void ioapic_redirect_gsi(uint32 lapic_id, uint8 vector, uint32 gsi, uint16 flags, uint8 mask) {
    uint32 ioapic = ioapic_get_gsi(gsi);
    uint64 redirect = (uint64)vector;

    if ((flags & (1 << 1)) != 0) {
        redirect |= (1 << 13);
    }

    if ((flags & (1 << 3)) != 0) {
        redirect |= (1 << 15);
    }

    if(mask == 0) {
        redirect &= 0 << 16;
    }else {
        redirect |= 1 << 16;
    }
    redirect |= (uint64)lapic_id << 56;

    uint32 redir_table = (gsi - madt_ioapic_list[ioapic]->gsi_base) * 2 + 16;

    write_ioapic(ioapic, redir_table, (uint32)redirect);
    write_ioapic(ioapic, redir_table + 1, (uint32)redirect >> 32);
}

void ioapic_redirect_irq(uint32 lapic_id, uint8 vector, uint8 irq, uint8 mask) {
    uint32 index = 0;
    madt_iso* iso;

    while (index < madt_iso_len) {
        iso = madt_iso_list[index];
        if (iso->irq_src == irq) {
            if (mask) {
                serial_printf("IRQ %x.8  override\n", irq);
            }
            ioapic_redirect_gsi(lapic_id, vector, iso->gsi, iso->flags, mask);
            return;
        }
        index++;
    }
    ioapic_redirect_gsi(lapic_id, vector, irq, 0, mask);
}

