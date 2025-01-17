//
// Created by dustyn on 8/3/24.
//

#pragma once

#include "include/definitions/types.h"
#include "include/architecture/x86_64/hpet.h"

struct acpi_rsdp {
    int8_t signature[8];
    uint8_t checksum;
    int8_t oem_id[6];
    uint8_t revision;
    uint32_t rsdt_addr;
    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t extended_checksum;
    uint8_t reserved[3];
}__attribute__((packed));

struct acpi_sdt {
    int8_t signature[4];
    uint32_t len;
    uint8_t revision;
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
}__attribute__((packed));

struct acpi_rsdt {
    struct acpi_sdt sdt;
    int8_t table[];
} __attribute__((packed));

#define NUM_MCFG_ENTRIES(header) ((header)->length - sizeof(struct mcfg_header)) / sizeof(struct mcfg_entry)

struct mcfg_entry {
    uint64_t base_address;
    uint16_t segment_group;
    uint8_t start_bus;
    uint8_t end_bus;
    uint32_t reserved;
}__attribute__((packed));

struct mcfg_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
    uint64_t reserved;
    struct mcfg_entry entry[];
}__attribute__((packed));

struct address_structure {
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved;
    uint64_t address;
} __attribute__((packed));


struct hpet {
    uint8_t hardware_rev_id;
    uint8_t comparator_count: 5;
    uint8_t counter_size: 1;
    uint8_t reserved: 1;
    uint8_t legacy_replacement: 1;
    uint16_t pci_vendor_id;
    struct address_structure address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed));

struct description_table_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemid[6];
    uint64_t oem_tableid;
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
    struct hpet hpet[];
} __attribute__((packed));


extern int8_t acpi_extended;
extern void *acpi_root_sdt;

void acpi_init();

void *find_acpi_table(const char *name);
