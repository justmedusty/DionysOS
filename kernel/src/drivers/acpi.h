//
// Created by dustyn on 8/3/24.
//

#pragma once
#include "include/types.h"


typedef struct {
char sign[8];
uint8 checksum;
char oem_id[6];
uint8 revision;
uint32 rsdt_addr;
}__attribute__((packed)) acpi_rsdp;

typedef struct {
    char sign[8];
    uint8 checksum;
    int8 oem_id[6];
    uint8 revision;
    uint32 resv;
    uint32 length;
    uint64 xsdt_addr;
    uint8 extended_checksum;
    uint8 resvl[3]
}__attribute__((packed)) acpi_xsdp;

typedef struct{
char sign[4];
uint32 len;
uint8 revision;
uint8 checksum;
uint8 oem_id[6];
uint32 oem_revision;
uint32 creator_revision;
}__attribute__((packed)) acpi_sdt;

typedef struct{
acpi_sdt sdt;
int8 table[];
} acpi_rsdt;

typedef struct {
acpi_sdt sdt;
int8 table[];
}acpi_xsdt;

uint64 acpi_init();

void* find_acpi_table(const int8 *name);