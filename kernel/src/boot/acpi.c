//
// Created by dustyn on 8/3/24.
//

#include "include/acpi.h"
#include "include/uart.h"
#include "include/arch_paging.h"
#include "include/mem.h"
#include "include/uart.h"
#include "include/cpu.h"
#include "limine.h"

__attribute__((used, section(".requests")))
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0,
};

int8 acpi_extended = 0;

void* acpi_root_sdt;

void* find_acpi_table(const int8* name) {
  if (!acpi_extended) {
    acpi_rsdt* rsdt = (acpi_rsdt*)acpi_root_sdt;
    uint32 entries = (rsdt->sdt.len - sizeof(rsdt->sdt)) / 4;

    for (int i = 0; i < entries; i++) {
      acpi_sdt* sdt = (acpi_sdt*)P2V(*((uint32*)rsdt->table + i));
      if (!memcmp(sdt->sign, name, 4))
        return (void*)sdt;
    }
    return NULL;
  }

  // Use XSDT
  acpi_xsdt* xsdt = (acpi_xsdt*)acpi_root_sdt;
  uint32 entries = (xsdt->sdt.len - sizeof(xsdt->sdt)) / 8;

  for (int i = 0; i < entries; i++) {
    acpi_sdt* sdt = (acpi_sdt*)P2V(*((uint64*)xsdt->table + i));
    if (!memcmp(sdt->sign, name, 4)) {
      return (void*)sdt;
    }
  }

  return NULL;
}

uint64 acpi_init() {
  void* addr = (void*)rsdp_request.response->address;
  acpi_rsdp* rsdp = (acpi_rsdp*)addr;

  if (memcmp(rsdp->sign, "RSD PTR", 7)){
    serial_printf("Error occurred parsing rsdp table\n");
    return 0;
}
  if (rsdp->revision != 0) {
    // Use XSDT
    serial_printf("Using xsdt\n");
    acpi_extended = 1;
    acpi_xsdp* xsdp = (acpi_xsdp*)addr;
    acpi_root_sdt = (acpi_xsdt*)P2V((uint64)xsdp->xsdt_addr);
    return xsdp->xsdt_addr;
  }

  acpi_root_sdt = (acpi_rsdt*)P2V((uint64)rsdp->rsdt_addr);
  serial_printf("Using xsdt\n");
  return rsdp->rsdt_addr;
}