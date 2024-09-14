//
// Created by dustyn on 8/3/24.
//


#include "include/acpi.h"
#include "include/madt.h"
#include "include/uart.h"
#include <include/arch/arch_cpu.h>

madt_ioapic* madt_ioapic_list[32] = {0};
madt_lapic* madt_lapic_list[32] = {0};
madt_iso* madt_iso_list[32] = {0};
madt_nmi* madt_nmi_list[32] = {0};

uint32 madt_ioapic_len = 0;
uint32 madt_iso_len = 0;
uint32 madt_lapic_len = 0;
uint32 madt_nmi_len = 0;

void madt_init() {
  acpi_madt* madt = (acpi_madt*)find_acpi_table("APIC");
  
  if(madt == 0){
    panic("No MADT!");
  }

  uint64 offset = 0;
  uint64 current_idx = 0;
  madt_ioapic_len = 0;
  madt_lapic_len = 0;
  madt_iso_len = 0;


  while (1) {
    madt_header* header = (madt_header*)(madt->table + offset);
    if (offset > madt->header.len - (sizeof(madt) - 1))
      break;

    switch (header->type) {
      case 0:
        madt_lapic_list[madt_lapic_len++] = (madt_lapic*) header;
        serial_printf("Found local APIC\n");
        break;
      case 1:
        serial_printf("Found IOAPIC\n");
        madt_ioapic_list[madt_ioapic_len++] = (madt_ioapic*)header;
        break;
      case 2:
        serial_printf("Found MADT ISO\n");
        madt_iso_list[madt_iso_len++] = (madt_iso*)header;
        break;
      case 4:
        serial_printf("Found NMI\n");
        madt_nmi_list[madt_nmi_len++] = (madt_nmi*)header;
        break;
        case 9:
          serial_printf("Found x2apic, ignoring\n");
          break;
    }
    offset += header->len;
  }


}