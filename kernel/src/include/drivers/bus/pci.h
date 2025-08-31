//
// Created by dustyn on 12/19/24.
//

#ifndef KERNEL_PCI_H
#define KERNEL_PCI_H
#pragma once

#include "include/architecture/arch_cpu.h"
#include <stdint.h>
#include <stdbool.h>
#include "include/device/device.h"

#ifdef __x86_64__
#endif
#define PCI_MAX_BUSES 256
#define SLOTS_PER_BUS 32
#define FUNCTIONS_PER_DEVICE 8
#define CONFIG_SPACE_PER_DEVICE 4096


#define PCI_COMMAND_BUS_MASTER 0x04
//PCI Types
#define PCI_TYPE_GENERIC_DEVICE 0x0
#define PCI_TYPE_BRIDGE 0x1

// PCI Device Common Header Offsets
#define PCI_DEVICE_VENDOR_ID_OFFSET      0x00
#define PCI_DEVICE_ID_OFFSET             0x02
#define PCI_DEVICE_COMMAND_OFFSET        0x04
#define PCI_DEVICE_STATUS_OFFSET         0x06
#define PCI_DEVICE_REVISION_OFFSET       0x08
#define PCI_DEVICE_PROG_IF_OFFSET        0x09
#define PCI_DEVICE_SUB_CLASS_OFFSET      0x0A
#define PCI_DEVICE_CLASS_OFFSET          0x0B
#define PCI_DEVICE_CACHE_LINE_SIZE_OFFSET 0x0C
#define PCI_DEVICE_LATENCY_TIMER_OFFSET  0x0D
#define PCI_DEVICE_HEADER_TYPE_OFFSET    0x0E
#define PCI_DEVICE_BIST_OFFSET           0x0F

// PCI Generic Device Header Offsets (type 0x0)
#define PCI_GENERIC_BAR0_OFFSET          0x10
#define PCI_GENERIC_BAR1_OFFSET          0x14
#define PCI_GENERIC_BAR2_OFFSET          0x18
#define PCI_GENERIC_BAR3_OFFSET          0x1C
#define PCI_GENERIC_BAR4_OFFSET          0x20
#define PCI_GENERIC_BAR5_OFFSET          0x24
#define PCI_GENERIC_CARDBUS_CIS_OFFSET   0x28
#define PCI_GENERIC_SUB_VENDOR_OFFSET    0x2C
#define PCI_GENERIC_SUB_DEVICE_OFFSET    0x2E
#define PCI_GENERIC_EXPANSION_ROM_OFFSET 0x30
#define PCI_GENERIC_CAPABILITIES_OFFSET  0x34
#define PCI_GENERIC_INT_LINE_OFFSET      0x3C
#define PCI_GENERIC_INT_PIN_OFFSET       0x3D
#define PCI_GENERIC_MIN_GRANT_OFFSET     0x3E
#define PCI_GENERIC_MAX_LATENCY_OFFSET   0x3F

// PCI-to-PCI Bridge Header Offsets (type 0x1)
#define PCI_BRIDGE_BAR0_OFFSET           0x10
#define PCI_BRIDGE_BAR1_OFFSET           0x14
#define PCI_BRIDGE_PRIMARY_BUS_OFFSET    0x18
#define PCI_BRIDGE_SECONDARY_BUS_OFFSET  0x19
#define PCI_BRIDGE_SUBORDINATE_BUS_OFFSET 0x1A
#define PCI_BRIDGE_LATENCY_TIMER_OFFSET  0x1B
#define PCI_BRIDGE_IO_BASE_OFFSET        0x1C
#define PCI_BRIDGE_IO_LIMIT_OFFSET       0x1D
#define PCI_BRIDGE_STATUS2_OFFSET        0x1E
#define PCI_BRIDGE_MEM_BASE_OFFSET       0x20
#define PCI_BRIDGE_MEM_LIMIT_OFFSET      0x22
#define PCI_BRIDGE_PREFETCH_BASE_OFFSET  0x24
#define PCI_BRIDGE_PREFETCH_LIMIT_OFFSET 0x26
#define PCI_BRIDGE_PREFETCH_BASE_UPPER_OFFSET 0x28
#define PCI_BRIDGE_PREFETCH_LIMIT_UPPER_OFFSET 0x2C
#define PCI_BRIDGE_IO_BASE_UPPER_OFFSET  0x30
#define PCI_BRIDGE_IO_LIMIT_UPPER_OFFSET 0x32
#define PCI_BRIDGE_CAPABILITIES_OFFSET   0x34
#define PCI_BRIDGE_EXPANSION_ROM_OFFSET  0x38
#define PCI_BRIDGE_INT_LINE_OFFSET       0x3C
#define PCI_BRIDGE_INT_PIN_OFFSET        0x3D
#define PCI_BRIDGE_CONTROL_OFFSET        0x3E


#define PCI_CONFIG_ADD 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define PCI_CLASS_UNASSIGNED 0xFF
#define PCI_DEVICE_DOESNT_EXIST 0xFFFF
#define PCI_ID_MASK PCI_DEVICE_DOESNT_EXIST
#define BYTE_MASK 0xFF
#define SHORT_MASK 0xFFFF
#define WORD_MASK 0xFFFFFFFF

#define BUS_SHIFT 16
#define SLOT_SHIFT 11
#define FUNCTION_SHIFT 8

#define PCI_BUS_MASK 0xFF
#define PCI_SLOT_MASK 0x1F
#define PCI_FUNCTION_MASK 0x7
#define PCI_REG_MASK 0xFC


#define PCI_BASE_ADDRESS_SPACE_IO 0x01
#define PCI_BASE_ADDRESS_IO_MASK (~0x03ULL)
#define PCI_BASE_ADDRESS_MEM_MASK (~0x0FULL)

/*
 * PCI driver abstraction, taking the simpler stuff out of the linux implementation
 */
struct pci_driver {
    char *name;
    const struct pci_device *device;

    int32_t (*probe)(struct device *device);

    void (*remove)(struct device *device);

    int32_t (*suspend)(struct device *device);

    int32_t (*resume)(struct device *device);

    uint32_t (*shutdown)(struct device *device);

    void (*bind)(struct device *device);

    // Skip the SRIOV stuff since I don't plan on supporting virtualization of devices
    bool driver_managed_dma;

};

struct pci_controller {
    struct device *bus;
    struct device *controller;

    uint32_t first_bus;
    uint32_t last_bus;

    volatile uint32_t configuration_address;
    volatile int8_t *configuration_data;
};

struct pci_bus_information {
    uintptr_t pci_mmio_address;
    uint8_t start_bus;
    uint8_t end_bus;
};

// Generic PCI Device AKA not a bridge
struct generic_pci_device {
    uint32_t base_address_registers[6];
    uint16_t sub_vendor;
    uint16_t sub_device;
    uint32_t expansion_rom;
    uint8_t capabilities;
    uint8_t int_line;
    uint8_t min_grant;
    uint8_t max_latency;

};

// Bridge device, can only be one or the other
struct pci_bridge {
    uint32_t base_address_registers[2];
    uint8_t bus_primary;
    uint8_t bus_secondary;
    uint8_t bus_subordinate;
    uint8_t latency_timer2;
    uint8_t io_base;
    uint8_t io_limit;
    uint16_t status2;
    uint16_t mem_base;
    uint16_t mem_limit;
    uint16_t pre_base;
    uint16_t pre_limit;
    uint32_t pre_base_upper;
    uint32_t pre_limit_upper;
    uint16_t io_base_upper;
    uint16_t io_limit_upper;
    uint8_t capabilities;
    uint32_t expansion_rom;
    uint8_t int_line;
    uint8_t int_pin;
    uint16_t bridge_control;
};

struct pci_device {
    uint16_t bus;
    uint16_t slot;
    uint8_t function;
    uint8_t prog_if;
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t revision_id;
    uint8_t header_type;
    uint16_t command;
    uint16_t status;
    uint8_t cache_line_size;
    uint8_t class;
    uint8_t subclass;
    bool mutlifunction;
    bool sixtyfour_bit_bar;
    uint16_t msi_offset;
    bool msi_support;
    bool registered;
    union {
        struct generic_pci_device generic;
        struct pci_bridge bridge;
    };
    struct pci_slot *pci_slot;
    struct pci_driver *driver;
};

struct pci_slot {
    uint8_t id;
    struct pci_device pci_devices[8];
    struct pci_bus *bus;
};


struct pci_bus {
    uint8_t bus_id;
    struct pci_slot pci_slots[SLOTS_PER_BUS];
};

struct pci_bus_information get_pci_info();

void pci_enumerate_devices(bool print);

char *pci_get_class_name(uint8_t class);

char *pci_get_subclass_name(uint8_t class, uint8_t subclass);

uint32_t pci_read_base_address_register(struct device *device, int32_t base_address_register_number);

void pci_write_base_address_register(struct device *device, int32_t base_address_register_number, uint32_t address);

void pci_map_bar(uint64_t bar_phys_addr, uint64_t *pgdir, uint8_t permissions, uint64_t pages);

#ifdef __x86_64__

#include "include/architecture/x86_64/acpi.h"

void set_pci_mmio_address(struct mcfg_entry *entry);

void pci_get_address(struct pci_device *device, uint32_t offset);

void pci_enable_bus_mastering(struct pci_device *device);
// Message Signaled Interrupts (MSI) format message flags
#define MSI_EDGE_TRIGGER  (1 << 15)  // Edge-triggered interrupt bit
#define MSI_DEASSERT      (1 << 14)  // Deassert interrupt bit
#define MSI_BASE_ADDR     0xfee00000 // Base address for MSI

static inline uint64_t msi_format_message(uint32_t *data, int32_t vector, bool edgetrigger, bool deassert) {
    *data = (edgetrigger ? 0 : MSI_EDGE_TRIGGER) |
            (deassert ? 0 : MSI_DEASSERT) |
            vector;
    return MSI_BASE_ADDR | (my_cpu()->cpu_id << 12);
}

#endif

#endif //KERNEL_PCI_H
