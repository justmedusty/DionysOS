//
// Created by dustyn on 8/3/24.
//
#pragma once
#include "include/types.h"
#include "acpi.h"

typedef struct {
  int8 sign[4];
  uint32 len;
  uint8 revision;
  uint8 checksum;
  int8 oem_id[6];
  int8 oem_table_id[8];
  uint32 oem_revision;
  uint32 creator_id;
  uint32 creator_revision;

  /* MADT Specs */
  uint32 lapic_address;
  uint32 flags;

  int8 table[];
} acpi_madt;

typedef struct {
  uint8 type;
  uint8 len;
} madt_entry;

typedef struct {
  madt_entry un;
  uint8 cpu_id;
  uint8 apic_id;
  uint32 flags;
} madt_cpu_lapic;

typedef struct {
  madt_entry un;
  uint8 apic_id;
  uint8 resv;
  uint32 apic_addr;
  uint32 gsi_base;
} madt_ioapic;

typedef struct {
  madt_entry un;
  uint8 bus_src;
  uint8 irq_src;
  uint32 gsi;
  uint16 flags;
} madt_iso;

typedef struct {
  madt_entry un;
  uint16 resv;
  uint64 phys_lapic;
} madt_lapic_addr;

extern madt_ioapic* madt_ioapic_list[128];
extern madt_iso* madt_iso_list[128];

extern uint32 madt_ioapic_len;
extern uint32 madt_iso_len;

extern uint64* lapic_addr;

void madt_init();
