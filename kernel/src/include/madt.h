//
// Created by dustyn on 8/3/24.
//
#pragma once
#include "include/types.h"
#include "acpi.h"

typedef struct {
  uint32 lapic_address;
  uint32 flags;
  int8 table[];
}__attribute__((packed))  acpi_madt;

typedef struct {
  uint8 type;
  uint8 len;
}__attribute__((packed))  madt_header;

typedef struct {
  madt_header header;
  uint8 cpu_id;
  uint8 apic_id;
  uint32 flags;
}__attribute__((packed))  madt_cpu_lapic;

typedef struct {
  madt_header header;
  uint8 apic_id;
  uint8 resv;
  uint32 apic_addr;
  uint32 gsi_base;
} __attribute__((packed)) madt_ioapic;

typedef struct {
  madt_header header;
  uint8 bus_src;
  uint8 irq_src;
  uint32 gsi;
  uint16 flags;
}__attribute__((packed))  madt_iso;

typedef struct {
  madt_header header;
  uint16 resv;
  uint64 phys_lapic;
}__attribute__((packed))  madt_lapic_addr;

extern madt_ioapic* madt_ioapic_list[128];
extern madt_iso* madt_iso_list[128];

extern uint32 madt_ioapic_len;
extern uint32 madt_iso_len;

extern uint64* lapic_addr;

void madt_init();
