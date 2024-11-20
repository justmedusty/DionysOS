//
// Created by dustyn on 8/3/24.
//
#pragma once
#include "include/types.h"
#include "acpi.h"

typedef struct {
  acpi_sdt header;
  uint32_t lapic_address;
  uint32_t flags;
  int8_t table[];
}__attribute__((packed))  acpi_madt;

typedef struct {
  uint8_t type;
  uint8_t len;
}__attribute__((packed))  madt_header;

typedef struct {
  madt_header header;
  uint8_t cpu_id;
  uint8_t apic_id;
  uint32_t flags;
}__attribute__((packed))  madt_lapic;

typedef struct {
  madt_header header;
  uint8_t apic_id;
  uint8_t resv;
  uint32_t apic_addr;
  uint32_t gsi_base;
} __attribute__((packed)) madt_ioapic;

typedef struct {
  madt_header header;
  uint8_t bus_src;
  uint8_t irq_src;
  uint32_t gsi;
  uint16_t flags;
}__attribute__((packed))  madt_iso;

typedef struct {
  madt_header header;
  uint16_t resv;
  uint64_t phys_lapic;
}__attribute__((packed))  madt_lapic_addr;

typedef struct {
  madt_header header;
  uint8_t processor;
  uint16_t flags;
  uint8_t lint;
}__attribute__((packed)) madt_nmi;

extern madt_ioapic* madt_ioapic_list[32];
extern madt_lapic* madt_lapic_list[32];
extern madt_iso* madt_iso_list[32];
extern madt_nmi* madt_nmi_list[32];

extern uint32_t madt_ioapic_len;
extern uint32_t madt_iso_len;
extern uint32_t madt_lapic_len;

void madt_init();
