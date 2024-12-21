//
// Created by dustyn on 8/3/24.
//

#include "include/architecture/x86_64/acpi.h"
#include "include/drivers/serial/uart.h"
#include "include/architecture//arch_paging.h"
#include "include/memory/mem.h"
#include "include/drivers/serial/uart.h"
#include <include/architecture/arch_cpu.h>
#include "include/device/bus/pci.h"
#include "include/architecture/x86_64/madt.h"
#include "limine.h"

__attribute__((used, section(".requests")))
static volatile struct limine_rsdp_request rsdp_request = {
        .id = LIMINE_RSDP_REQUEST,
        .revision = 0,
};

int8_t acpi_extended = 0;

void *acpi_root_sdt;

void *find_acpi_table(const char *name) {
    struct acpi_rsdt *rsdt = (struct acpi_rsdt *) acpi_root_sdt;
    if (!acpi_extended) {
        uint32_t entries = (rsdt->sdt.len - sizeof(rsdt->sdt)) / 4;

        for (uint32_t i = 0; i < entries; i++) {
            struct acpi_sdt *sdt = (struct acpi_sdt *) P2V(*((uint32_t *) rsdt->table + i));
            if (!memcmp(sdt->signature, name, 4))
                return (void *) sdt;
        }
        return NULL;
    }

    // Extended
    uint32_t entries = (rsdt->sdt.len - sizeof(rsdt->sdt)) / 8;

    for (uint32_t i = 0; i < entries; i++) {
        struct acpi_sdt *sdt = (struct acpi_sdt *) P2V(*((uint64_t *) rsdt->table + i));
        if (!memcmp(sdt->signature, name, 4)) {
            return (void *) sdt;
        }
    }

    panic("ACPI Table not found");
}

/*
 * Use the rsdp to find the r/xsdt and also find the mcfg table for setting up PCIe by taking the base address out of the table.
 * Finally parse the MADT to find information about ISOs, apics, NMI
 */
void acpi_init() {
    void *addr = (void *) rsdp_request.response->address;
    struct acpi_rsdp *rsdp = (struct acpi_rsdp *) addr;

    if (!rsdp) {
        panic("rsdp null");
    }

    serial_printf("acpi revision %x.8 \n", rsdp->revision);

    if (rsdp->revision != 0) {
        // Use XSDT
        serial_printf("Using xsdt\n");
        acpi_extended = 1;
        acpi_root_sdt = (struct acpi_rsdt *) P2V((uint64_t) rsdp->xsdt_addr);
        serial_printf("xsdt addr %x.64 \n", rsdp->xsdt_addr);

        if (rsdp->xsdt_addr == 0) {
            panic("acpi init failed\n");
        }
    } else {

        acpi_root_sdt = (struct acpi_rsdt *) P2V((uint32_t) rsdp->rsdt_addr);
        serial_printf("rsdt addr %x.32 \n", rsdp->rsdt_addr);

        if (rsdp->rsdt_addr == 0) {
            panic("acpi init failed\n");
        }
    }

    struct mcfg_header *mcfg_header = (struct mcfg_header *) find_acpi_table("MCFG");

    if (!mcfg_header) {
        panic("Cannot find mcfg header to set up PCI\n");
    }
    struct mcfg_header *header = find_acpi_table("MCFG");

    set_pci_mmio_address((struct mcfg_entry *) &mcfg_header->entry);
    pci_enumerate_devices(true);
    madt_init();
}
