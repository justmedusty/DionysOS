//
// Created by dustyn on 8/3/24.
//

#pragma once
#include "include/types.h"

typedef struct {
    int8 signature[8];
    uint8 checksum;
    int8 oem_id[6];
    uint8 revision;
    uint32 rsdt_addr;
    uint32 length;
    uint64 xsdt_addr;
    uint8 extended_checksum;
    uint8 reserved[3];
}__attribute__((packed)) acpi_rsdp;

typedef struct {
    int8 signature[4];
    uint32 len;
    uint8 revision;
    uint8 checksum;
    uint8 oem_id[6];
    uint8 oem_table_id[8];
    uint32 oem_revision;
    uint32 creator_id;
    uint32 creator_revision;
}__attribute__((packed)) acpi_sdt;

typedef struct {
    acpi_sdt sdt;
    int8 table[];
} __attribute__((packed))acpi_rsdt;


extern int8 acpi_extended;
extern void* acpi_root_sdt;
void acpi_init();
void* find_acpi_table(const int8* name);
