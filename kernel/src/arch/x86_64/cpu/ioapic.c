//
// Created by dustyn on 6/21/24.
//
#include <include/cpu.h>
#include "include/arch/arch_global_interrupt_controller.h"
#include <include/arch/arch_paging.h>
#include "include/arch/arch_asm_functions.h"
#include <include/uart.h>

void write_ioapic(madt_ioapic* ioapic, uint8 reg, uint32 value) {
    uint64* base = (uint64*)P2V((uint64)ioapic->apic_addr);
    mem_out((uint32*)base, reg);
    mem_out((uint32*)base + 16, value);
}

uint32 read_ioapic(madt_ioapic* ioapic, uint8 reg) {
    uint64* base = (uint64*)P2V((uint64)ioapic->apic_addr);
    mem_out((uint32*)base, reg);
    return mem_in((uint32*)base + 16);
}

void ioapic_set_entry(madt_ioapic* ioapic, uint8 index, uint64 data) {
    write_ioapic(ioapic, (uint8)(IOAPIC_REDTBL + index * 2), (uint32)data);
    write_ioapic(ioapic, (uint8)(IOAPIC_REDTBL + index * 2 + 1), (uint32)(data >> 32));
}

uint64 ioapic_gsi_count(uint32 ioapic) {
    return (read_ioapic(madt_ioapic_list[ioapic], 1) & 0xff0000) >> 16;
}

madt_ioapic* ioapic_get_gsi(uint32 gsi) {
    for (uint64 i = 0; i < madt_ioapic_len; i++) {
        madt_ioapic* ioapic = madt_ioapic_list[i];
        uint64 num = ioapic_gsi_count(i);
        serial_printf("madt len : %x.32 ioapic gsi base %x.32, apic id %x.8  apic address %x.32 gsi count %x.64\n",
                      madt_ioapic_len, ioapic->gsi_base, ioapic->apic_id, ioapic->apic_addr, num);
        if (ioapic->gsi_base <= gsi && ioapic->gsi_base + ioapic_gsi_count(i) > gsi)
            return ioapic;
    }
    panic("Cannot determine IOAPIC from GSI");
}

void ioapic_redirect_gsi(uint32 lapic_id, uint8 vector, uint32 gsi, uint16 flags, uint8 mask) {
    madt_ioapic* ioapic = ioapic_get_gsi(gsi);

    uint64 redirect = vector;

    if ((flags & (1 << 1)) != 0) {
        redirect |= (1 << 13);
    }

    if ((flags & (1 << 3)) != 0) {
        redirect |= (1 << 15);
    }

    if (!mask) {
        redirect |= (1 << 16);
    }

    redirect |= (uint64_t)lapic_id << 56;

    uint32 redir_table = (gsi - ioapic->gsi_base) * 2 + 16;
    write_ioapic(ioapic, redir_table, (uint32)redirect);
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


uint64 ioapic_init() {
    madt_ioapic* ioapic = madt_ioapic_list[0];

    uint32 val = read_ioapic(ioapic, IOAPIC_VERSION);
    uint32 count = val >> 16 & 0xFF;

    if (read_ioapic(ioapic, 0) >> 24 != ioapic->apic_id) {
        return 0;
    }

    for (uint8 i = 0; i <= count; ++i) {
        write_ioapic(ioapic, IOAPIC_REDTBL + 2 * i, 0x00010000 | 32 + i);
        write_ioapic(ioapic, IOAPIC_REDTBL + 2 * i + 1, 0); // redir cpu
    }

    serial_printf("Ioapic Initialised\n");

    return 1;
}
