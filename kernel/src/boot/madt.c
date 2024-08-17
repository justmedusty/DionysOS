//
// Created by dustyn on 8/3/24.
//


#include "include/acpi.h"
#include "include/madt.h"
#include "include/uart.h"
#include "include/cpu.h";

madt_ioapic* madt_ioapic_list[128] = {0};
madt_lapic* madt_lapic_list[128] = {0};
madt_iso* madt_iso_list[128] = {0};

uint32 madt_ioapic_len = 0;
uint32 madt_iso_len = 0;

uint64* lapic_addr = NULL;

void madt_init() {
  acpi_madt* madt = (acpi_madt*)find_acpi_table("APIC");
  
  if(madt == 0){
    panic("No MADT!");
  }

  uint64 offset = 0;
  uint64 current_idx = 0;
  madt_ioapic_len = 0;
  madt_iso_len = 0;



  while (1) {
    madt_header* header = (madt_header*)(madt->table + offset);
    if (offset > header->len - sizeof(madt))
      break;




    serial_printf("iso len %x.32 \n",header->len);
    switch (header->type) {
      case 0:
        madt_lapic_list[current_idx] = (madt_lapic*) header;

        current_idx++;
        serial_printf("Found local APIC\n");

        break;
      case 1:
        madt_ioapic_list[madt_ioapic_len++] = (madt_ioapic*)header;
        break;
      case 2:
        madt_iso_list[madt_iso_len++] = (madt_iso*)header;

        break;
      case 4:
        lapic_addr = (uint64*)((madt_lapic_addr*)header)->phys_lapic;
        break;
    }

    offset += header->len;
  }

}