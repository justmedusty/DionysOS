//
// Created by dustyn on 8/3/24.
//


#include "include/architecture/x86_64/acpi.h"
#include "include/architecture/x86_64/madt.h"
#include "include/drivers/serial/uart.h"
#include <include/architecture/arch_cpu.h>

struct madt_ioapic *madt_ioapic_list[32] = {0};
struct madt_lapic *madt_lapic_list[32] = {0};
struct madt_iso *madt_iso_list[32] = {0};
struct madt_nmi *madt_nmi_list[32] = {0};

uint32_t madt_ioapic_len = 0;
uint32_t madt_iso_len = 0;
uint32_t madt_lapic_len = 0;
uint32_t madt_nmi_len = 0;

void madt_init() {
    struct acpi_madt *madt = (struct acpi_madt *) find_acpi_table("APIC");

    if (madt == 0) {
        panic("No MADT!");
    }

    uint64_t offset = 0;
    uint64_t current_idx = 0;
    madt_ioapic_len = 0;
    madt_lapic_len = 0;
    madt_iso_len = 0;


    while (1) {
        struct madt_header *header = (struct madt_header *) (madt->table + offset);
        if (offset > madt->header.len - (sizeof(struct acpi_madt) - 1) || header->len == 0)
            break;
        switch (header->type) {
            case 0:
                madt_lapic_list[madt_lapic_len++] = (struct madt_lapic *) header;
                serial_printf("Found local APIC\n");
                break;
            case 1:
                serial_printf("Found IOAPIC\n");
                madt_ioapic_list[madt_ioapic_len++] = (struct madt_ioapic *) header;
                break;
            case 2:
                serial_printf("Found MADT ISO\n");
                madt_iso_list[madt_iso_len++] = (struct madt_iso *) header;
                break;
            case 4:
                serial_printf("Found NMI\n");
                madt_nmi_list[madt_nmi_len++] = (struct madt_nmi *) header;
                break;
            case 9:
                serial_printf("Found x2apic, ignoring\n");
                break;
            default:
                break;
        }
        offset += header->len;
    }


}