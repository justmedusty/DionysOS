//
// Created by dustyn on 9/29/24.
//

#ifndef AHCI_H
#define AHCI_H
#pragma once

#include <stdint.h>
#include <include/data_structures/doubly_linked_list.h>
#include <include/data_structures/spinlock.h>
#include "include/drivers/bus/pci.h"
// Device class/subclass for AHCI
#define AHCI_CLASS     0x01
#define AHCI_SUBCLASS  0x06
#define AHCI_PROGIF    0x01

// SATA signatures
#define SATA_ATA       0x00000101
#define SATA_ATAPI     0xEB140101
#define SATA_SEMB      0xC33C0101
#define SATA_PM        0x96690101

// HBA Port Command bits
#define HBA_CMD_ST     0x0001
#define HBA_CMD_FRE    0x0010
#define HBA_CMD_FR     0x4000
#define HBA_CMD_CR     0x8000

// FIS Types
#define FIS_TYPE_REG_H2D     0x27
#define FIS_TYPE_REG_D2H     0x34
#define FIS_TYPE_DMA_ENABLE  0x39
#define FIS_TYPE_DMA_SETUP   0x41
#define FIS_TYPE_DATA        0x46
#define FIS_TYPE_BIST        0x58
#define FIS_TYPE_PIO_SETUP   0x5F
#define FIS_TYPE_DEV_BITS    0xA1

#define READ 0x25
#define WRITE 0x35
// Sector size
#define SECTOR_SIZE 0x200

struct ahci_port_registers {
    volatile uint32_t command_list_base_address;
    volatile uint32_t command_list_base_address_upper;
    volatile uint32_t fis_base_address;
    volatile uint32_t fis_base_address_upper;
    volatile uint32_t interrupt_status;
    volatile uint32_t interrupt_enable;
    volatile uint32_t command_and_status;
    volatile uint32_t reserved0;
    volatile uint32_t task_file_data;
    volatile uint32_t signature;
    volatile uint32_t sata_status;
    volatile uint32_t sata_control;
    volatile uint32_t sata_error;
    volatile uint32_t sata_active;
    volatile uint32_t command_issue;
    volatile uint32_t sata_notification;
    volatile uint32_t fis_based_switching_control;
    volatile uint32_t device_sleep;
    volatile uint32_t reserved1[11];
    volatile uint32_t vendor_specific[10];
} __attribute__((packed));

struct ahci_registers {
    volatile uint32_t capabilities;
    volatile uint32_t global_host_control;
    volatile uint32_t interrupt_status;
    volatile uint32_t ports_implemented;
    volatile uint32_t version;
    volatile uint32_t command_completion_control;
    volatile uint32_t command_completion_ports;
    volatile uint32_t enclosure_management_location;
    volatile uint32_t enclosure_management_control;
    volatile uint32_t capabilities_extended;
    volatile uint32_t bios_os_handoff_control_status;
    volatile uint32_t reserved[29];
    volatile uint32_t vendor_specific[24];
} __attribute__((packed));


struct ahci_command_header {
    uint16_t flags;
    uint16_t prdt_entry_count;
    uint32_t byte_count_transferred;
    uint32_t command_table_base_address;
    uint32_t command_table_base_address_upper;
    uint32_t reserved[4];
} __attribute__((packed));

struct ahci_prdt_entry {
    uint32_t data_base_address;
    uint32_t data_base_address_upper;
    uint32_t reserved;
    uint32_t byte_count_and_interrupt_flag;
} __attribute__((packed));


struct ahci_command_table {
    uint8_t command_fis[64];
    uint8_t atapi_command[16];
    uint8_t reserved[48];
    struct ahci_prdt_entry prdt_entries[1];
} __attribute__((packed));


struct ahci_fis_host_to_device {
    uint8_t fis_type;
    uint8_t flags;
    uint8_t command;
    uint8_t feature_low;

    uint8_t lba_low;
    uint8_t lba_mid;
    uint8_t lba_high;
    uint8_t device;

    uint8_t lba_low_exp;
    uint8_t lba_mid_exp;
    uint8_t lba_high_exp;
    uint8_t feature_high;

    uint8_t sector_count_low;
    uint8_t sector_count_high;
    uint8_t iso_command_completion;
    uint8_t control;

    uint32_t reserved;
} __attribute__((packed));


struct ahci_fis_device_to_host {
    uint8_t fis_type;
    uint8_t port_and_reserved;
    uint8_t status;
    uint8_t error;

    uint8_t lba_low;
    uint8_t lba_mid;
    uint8_t lba_high;
    uint8_t device;

    uint8_t lba_low_exp;
    uint8_t lba_mid_exp;
    uint8_t lba_high_exp;
    uint8_t reserved1;

    uint8_t sector_count_low;
    uint8_t sector_count_high;
    uint8_t reserved2;
    uint8_t reserved3;
} __attribute__((packed));

struct ahci_controller {
    void *pci_bar;
    volatile struct ahci_registers *ahci_regs;
    uint32_t port_count;
    uint32_t command_slots;
    struct doubly_linked_list device_list;
    uint32_t major;
    uint32_t minor;
};
struct ahci_device {
    int32_t status;
    struct device *controller;
    volatile struct ahci_port_registers *registers;
    struct spinlock lock;
    struct ahci_controller *parent;
};

uint32_t ahci_find_command_slot(struct ahci_device *device);

volatile struct ahci_command_table *
set_prdt(volatile struct ahci_command_header *header, uint64_t buffer, uint32_t interrupt_vector, uint32_t byte_count);

void ahci_send_command(uint32_t slot, struct ahci_device *device);

int32_t ahci_initialize_device(struct ahci_device *device);

int32_t ahci_give_kernel_ownership(struct ahci_controller *controller);

void setup_ahci_device(struct pci_device *pci_device);

uint64_t ahci_write_block(uint64_t block_number, size_t block_count, char *buffer, struct device *device);

uint64_t ahci_read_block(uint64_t block_number, size_t block_count, char *buffer, struct device *device);

#endif //AHCI_H
