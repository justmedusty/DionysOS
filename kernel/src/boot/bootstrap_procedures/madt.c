//
// Created by dustyn on 8/3/24.
//


#include "acpi.h"
#include "madt.h"


madt_ioapic* madt_ioapic_list[128] = {0};
madt_iso* madt_iso_list[128] = {0};

uint32 madt_ioapic_len = 0;
uint32 madt_iso_len = 0;

uint64* lapic_addr = NULL;

void madt_init() {
  acpi_madt* madt = (acpi_madt*)find_acpi_table("APIC");

  uint64 offset = 0;
  int current_idx = 0;
  madt_ioapic_len = 0;
  madt_iso_len = 0;

  while (1) {
    if (offset > madt->len - sizeof(madt))
      break;
    
    madt_entry* entry = (madt_entry*)(madt->table + offset);

    switch (entry->type) {
      case 0:
        current_idx++;
        break;
      case 1:
        madt_ioapic_list[madt_ioapic_len++] = (madt_ioapic*)entry;
        break;
      case 2:
        madt_iso_list[madt_iso_len++] = (madt_iso*)entry;
        break;
      case 5:
        lapic_addr = (uint64*)((madt_lapic_addr*)entry)->phys_lapic;
        break;
    }

    offset += entry->len;
  }

}