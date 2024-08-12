//
// Created by dustyn on 6/21/24.
//
#include <include/cpu.h>

#include "include/arch/arch_global_interrupt_controller.h"


#include <include/arch/arch_paging.h>
#include <include/uart.h>

void write_ioapic(madt_ioapic* ioapic, uint8 reg, uint32 value) {
    *((volatile uint32*)P2V(ioapic->apic_addr) + IOAPIC_REG_SELECT) = reg;
    *((volatile uint32*)P2V(ioapic->apic_addr) + IOAPIC_IOWIN) = value;
}

uint32 read_ioapic(madt_ioapic* ioapic, uint8 reg) {
    *((volatile uint32*)P2V(ioapic->apic_addr) + IOAPIC_REG_SELECT) = reg;
    return *((volatile uint32*)P2V(ioapic->apic_addr) + IOAPIC_IOWIN);
}

void ioapic_set_entry(madt_ioapic* ioapic, uint8 index, uint64 data) {
    write_ioapic(ioapic, (uint8)(IOAPIC_REDTBL + index * 2), (uint32)data);
    write_ioapic(ioapic, (uint8)(IOAPIC_REDTBL + index * 2 + 1), (uint32)(data >> 32));
}

uint64 ioapic_gsi_count(madt_ioapic* ioapic) {
    return (read_ioapic(ioapic, 1) & 0xff0000) >> 16;
}

madt_ioapic* ioapic_get_gsi(uint32 gsi) {
    for (uint64 i = 0; i < madt_ioapic_len; i++) {
        madt_ioapic* ioapic = madt_ioapic_list[i];
        if (ioapic->gsi_base <= gsi && ioapic->gsi_base + ioapic_gsi_count(ioapic) > gsi)
            return ioapic;
    }
    return NULL;
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

    if (mask) redirect |= (1 << 16);
    else redirect &= ~(1 << 16);

    redirect |= (uint64_t)lapic_id << 56;

    //this line is page faulting not sure why
    uint32 redir_table = ((gsi - ioapic->gsi_base) * 2 + 16);
    panic("");
    write_ioapic(ioapic, redir_table, redirect);
    write_ioapic(ioapic, redir_table + 1, redirect >> 32);
}

void ioapic_redirect_irq(uint32 lapic_id, uint8 vector, uint8 irq, uint8 mask) {

    uint32 index = 0;
    madt_iso* iso = NULL;

    while (index < madt_iso_len) {
        iso = madt_iso_list[index];
        if (iso->irq_src == irq) {

            ioapic_redirect_gsi(lapic_id, vector, iso->gsi, iso->flags, mask);
            return;
        }
        index++;
    }
}


uint32 ioapic_get_redirect_irq(uint8 irq) {
    uint8 index = 0;
    madt_iso* iso;

    while (index < madt_iso_len) {
        iso = madt_iso_list[index];
        if (iso->irq_src == irq) {
            return iso->gsi;
        }
        index++;
    }
}

uint64 ioapic_init() {
    madt_ioapic* ioapic = madt_ioapic_list[0];

    uint32 val = read_ioapic(ioapic, IOAPIC_VERSION);
    uint32 count = val >> 16 & 0xFF;

    if (read_ioapic(ioapic, 0) >> 24 != ioapic->apic_id) {
        return 0;
    }

    for (uint8 i = 0; i <= count; ++i) {
        write_ioapic(ioapic, IOAPIC_REDTBL + 2 * i, 0x00010000 | (32 + i));
        write_ioapic(ioapic, IOAPIC_REDTBL + 2 * i + 1, 0); // redir cpu
    }

    serial_printf("Ioapic Initialised\n");

    return 1;
}
