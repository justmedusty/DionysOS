//
// Created by dustyn on 6/21/24.
//
#include <include/cpu.h>
#include "include/arch/arch_global_interrupt_controller.h"
#include <include/arch/arch_paging.h>
#include "include/arch/x86_64/arch_asm_functions.h"
#include <include/uart.h>

void write_ioapic(uint32 ioapic, uint32 reg, uint32 value) {
    uint64* base = P2V(madt_ioapic_list[ioapic]->apic_addr);

    mem_out(base, reg);
    mem_out(base + 16, value);
}

uint32 read_ioapic(uint32 ioapic, uint32 reg) {
    uint64* base = (uint64 *)P2V(madt_ioapic_list[ioapic]->apic_addr);
    mem_out(base, reg);
    return mem_in((uint64)base + 16);
}

uint64 ioapic_gsi_count(uint32 ioapic) {
    if (ioapic > madt_ioapic_len) {
        return 0;
    }
    uint64 value = (read_ioapic(ioapic, 1) & 0xff0000) >> 16;
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

    if (!mask) {
        redirect |= (1 << 16);
    }
    redirect |= (uint64)lapic_id << 56;

    uint32 redir_table = (gsi - madt_ioapic_list[ioapic]->gsi_base) * 2 + 16;

    write_ioapic(ioapic, redir_table, redirect);
    write_ioapic(ioapic, redir_table + 1, redirect >> 32);
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

