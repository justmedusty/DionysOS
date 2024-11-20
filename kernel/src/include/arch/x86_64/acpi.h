//
// Created by dustyn on 8/3/24.
//

#pragma once
#include "include/types.h"

typedef struct {
    int8_t signature[8];
    uint8_t checksum;
    int8_t oem_id[6];
    uint8_t revision;
    uint32_t rsdt_addr;
    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t extended_checksum;
    uint8_t reserved[3];
}__attribute__((packed)) acpi_rsdp;

typedef struct {
    int8_t signature[4];
    uint32_t len;
    uint8_t revision;
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
}__attribute__((packed)) acpi_sdt;

typedef struct {
    acpi_sdt sdt;
    int8_t table[];
} __attribute__((packed))acpi_rsdt;


extern int8_t acpi_extended;
extern void* acpi_root_sdt;
void acpi_init();
void* find_acpi_table(const int8_t* name);
