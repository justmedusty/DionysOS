//
// Created by dustyn on 12/19/24.
//

#ifndef KERNEL_PCI_H
#define KERNEL_PCI_H
#pragma once
#include <include/arch/x86_64/acpi.h>
#include <stdint.h>
#include <stdbool.h>

#define PCI_MAX_BUSES 256
#define SLOTS_PER_BUS 32
#define FUNCTIONS_PER_DEVICE 8
#define CONFIG_SPACE_PER_DEVICE 4096

#define PCI_DEVICE_VENDOR_ID_OFFSET 0x00
#define PCI_DEVICE_ID_OFFSET 0x02
#define PCI_DEVICE_TYPE_OFFSET 0x0E

#define PCI_CONFIG_ADD 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define PCI_DEVICE_DOESNT_EXIST 0xFFFF
#define PCI_ID_MASK PCI_DEVICE_DOESNT_EXIST

#define BUS_SHIFT 16
#define DEVICE_SHIFT 8
#define FUNCTION_SHIFT 8

#define PCI_BUS_MASK 0xFF
#define PCI_DEVICE_MASK 0x1F
#define PCI_FUNCTION_MASK 0x7
#define PCI_REG_MASK 0xFC

struct pci_driver {

};


struct generic_pci_device {
    uint32_t base_address_registers[6];
    uint32_t cardbus_cis;
    uint16_t sub_vendor;
    uint16_t sub_device;
    uint32_t expansion_rom;
    uint8_t capabilities;
    uint8_t int_line;
    uint8_t min_grant;
    uint8_t max_latency;
};


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
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t revision_id;
    uint8_t class;
    uint8_t subclass;
    bool mutlifunction;
    uint16_t msi_offset;
    bool msi_support;
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


void pci_scan(bool print);
char* pci_get_class_name(uint8_t class);
void set_pci_mmio_address(struct mcfg_entry *entry);
#endif //KERNEL_PCI_H
