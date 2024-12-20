//
// Created by dustyn on 12/19/24.
//

#include "include/dev/bus/pci.h"
#include <include/arch/x86_64/acpi.h>
#include"include/data_structures/doubly_linked_list.h"
#include "include/definitions.h"
#include "include/mem/kalloc.h"
#include "include/drivers/serial/uart.h"
#include "include/definitions/string.h"
#include "include/mem/mem.h"

/*
 * PCIe Functionality , the MMIO address is obtained via the MCFG table via an acpi table lookup
 */


struct pci_bus pci_buses[PCI_MAX_BUSES] = {0};

uintptr_t pci_mmio_address = 0;
uint8_t start_bus = 0;
uint8_t end_bus = 0;

/*
 * PCIe still uses the oldschool bus:slot:function layout so we use the those values + the shift macros in order to
 */
static uint32_t pci_read_config(struct pci_device *device,uint16_t offset) {
    uintptr_t address = (uintptr_t) P2V(pci_mmio_address)
                        + (uintptr_t) (device->bus << BUS_SHIFT)
                        + (uintptr_t) (device->slot << SLOT_SHIFT)
                        + (uintptr_t) (device->function << FUNCTION_SHIFT)
                        + (uintptr_t) offset;

    return *(volatile uint32_t *) address;
}


void set_pci_mmio_address(struct mcfg_entry *entry){
    pci_mmio_address = entry->base_address;
    start_bus = entry->start_bus;
    end_bus = entry->end_bus;
    serial_printf("PCI MMIO address is %x.64 , start bus %i end bus %i\n",pci_mmio_address,start_bus,end_bus);
}


void pci_enumerate_devices(bool print){
    for(uint16_t bus = 0; bus < PCI_MAX_BUSES; bus++){
        for(uint8_t slot = 0; slot < SLOTS_PER_BUS; slot++){
            for(uint8_t function = 0; function < FUNCTIONS_PER_DEVICE; function++){

                struct pci_bus *p_bus = &pci_buses[bus];
                struct pci_slot *p_slot = &p_bus->pci_slots[slot];
                struct pci_device *pci_device = &p_slot->pci_devices[function];

                if(pci_device->registered){
                    continue;
                }

                pci_device->bus = bus;
                pci_device->slot = slot;
                pci_device->function = function;

                uint16_t vendor_id = pci_read_config(pci_device,PCI_DEVICE_VENDOR_ID_OFFSET) & SHORT_MASK;

                if(vendor_id == PCI_DEVICE_DOESNT_EXIST){
                    if(function == 0){
                        memset(pci_device,0,sizeof(struct pci_device));
                        break;
                    }
                    memset(pci_device,0,sizeof(struct pci_device));
                    continue;
                }



                uint8_t class = pci_read_config(pci_device,PCI_DEVICE_CLASS_OFFSET) & BYTE_MASK;

                if(class == PCI_CLASS_UNASSIGNED || strcmp(pci_get_class_name(class),"Unknown")){
                    memset(pci_device,0,sizeof(struct pci_device));
                    continue;
                }

                pci_device->bus = bus;
                pci_device->slot = slot;
                pci_device->function = function;
                pci_device->vendor_id = vendor_id;
                pci_device->class = class;
                pci_device->device_id = pci_read_config(pci_device,PCI_DEVICE_ID_OFFSET) & SHORT_MASK;
                pci_device->command = pci_read_config(pci_device,PCI_DEVICE_COMMAND_OFFSET) & SHORT_MASK;
                pci_device->revision_id = pci_read_config(pci_device,PCI_DEVICE_REVISION_OFFSET) & SHORT_MASK;
                pci_device->header_type =  pci_read_config(pci_device,PCI_DEVICE_HEADER_TYPE_OFFSET) & BYTE_MASK;
                pci_device->class = class;

                switch(pci_device->header_type){
                    case PCI_TYPE_BRIDGE:
                        pci_device->bridge.base_address_registers[0] = pci_read_config(pci_device, PCI_BRIDGE_BAR0_OFFSET) & WORD_MASK;
                        pci_device->bridge.base_address_registers[1] = pci_read_config(pci_device, PCI_BRIDGE_BAR1_OFFSET) & WORD_MASK;
                        pci_device->bridge.bus_primary = pci_read_config(pci_device, PCI_BRIDGE_PRIMARY_BUS_OFFSET) & BYTE_MASK;
                        pci_device->bridge.bus_secondary = pci_read_config(pci_device, PCI_BRIDGE_SECONDARY_BUS_OFFSET) & BYTE_MASK;
                        pci_device->bridge.bus_subordinate = pci_read_config(pci_device, PCI_BRIDGE_SUBORDINATE_BUS_OFFSET) & BYTE_MASK;
                        pci_device->bridge.latency_timer2 = pci_read_config(pci_device, PCI_BRIDGE_LATENCY_TIMER_OFFSET) & BYTE_MASK;
                        pci_device->bridge.io_base = pci_read_config(pci_device, PCI_BRIDGE_IO_BASE_OFFSET) & BYTE_MASK;
                        pci_device->bridge.io_limit = pci_read_config(pci_device, PCI_BRIDGE_IO_LIMIT_OFFSET) & BYTE_MASK;
                        pci_device->bridge.status2 = pci_read_config(pci_device, PCI_BRIDGE_STATUS2_OFFSET) & SHORT_MASK;
                        pci_device->bridge.mem_base = pci_read_config(pci_device, PCI_BRIDGE_MEM_BASE_OFFSET) & SHORT_MASK;
                        pci_device->bridge.mem_limit = pci_read_config(pci_device, PCI_BRIDGE_MEM_LIMIT_OFFSET) & SHORT_MASK;
                        pci_device->bridge.pre_base = pci_read_config(pci_device, PCI_BRIDGE_PREFETCH_BASE_OFFSET) & SHORT_MASK;
                        pci_device->bridge.pre_limit = pci_read_config(pci_device, PCI_BRIDGE_PREFETCH_LIMIT_OFFSET) & SHORT_MASK;
                        pci_device->bridge.pre_base_upper = pci_read_config(pci_device, PCI_BRIDGE_PREFETCH_BASE_UPPER_OFFSET) & WORD_MASK;
                        pci_device->bridge.pre_limit_upper = pci_read_config(pci_device, PCI_BRIDGE_PREFETCH_LIMIT_UPPER_OFFSET) & WORD_MASK;
                        pci_device->bridge.io_base_upper = pci_read_config(pci_device, PCI_BRIDGE_IO_BASE_UPPER_OFFSET) & SHORT_MASK;
                        pci_device->bridge.io_limit_upper = pci_read_config(pci_device, PCI_BRIDGE_IO_LIMIT_UPPER_OFFSET) & SHORT_MASK;
                        pci_device->bridge.capabilities = pci_read_config(pci_device, PCI_BRIDGE_CAPABILITIES_OFFSET) & BYTE_MASK;
                        pci_device->bridge.expansion_rom = pci_read_config(pci_device, PCI_BRIDGE_EXPANSION_ROM_OFFSET) & WORD_MASK;
                        pci_device->bridge.int_line = pci_read_config(pci_device, PCI_BRIDGE_INT_LINE_OFFSET) & BYTE_MASK;
                        pci_device->bridge.int_pin = pci_read_config(pci_device, PCI_BRIDGE_INT_PIN_OFFSET) & BYTE_MASK;
                        pci_device->bridge.bridge_control = pci_read_config(pci_device, PCI_BRIDGE_CONTROL_OFFSET) & SHORT_MASK;
                        break;
                    case PCI_TYPE_GENERIC_DEVICE:
                        pci_device->generic.base_address_registers[0] = pci_read_config(pci_device,PCI_GENERIC_BAR0_OFFSET) & WORD_MASK;
                        pci_device->generic.base_address_registers[1] = pci_read_config(pci_device,PCI_GENERIC_BAR1_OFFSET) & WORD_MASK;
                        pci_device->generic.base_address_registers[2] = pci_read_config(pci_device,PCI_GENERIC_BAR2_OFFSET) & WORD_MASK;
                        pci_device->generic.base_address_registers[3] = pci_read_config(pci_device,PCI_GENERIC_BAR3_OFFSET) & WORD_MASK;
                        pci_device->generic.base_address_registers[4] = pci_read_config(pci_device,PCI_GENERIC_BAR4_OFFSET) & WORD_MASK;
                        pci_device->generic.base_address_registers[5] = pci_read_config(pci_device,PCI_GENERIC_BAR5_OFFSET) & WORD_MASK;
                        pci_device->generic.capabilities = pci_read_config(pci_device,PCI_GENERIC_CAPABILITIES_OFFSET) & BYTE_MASK;
                        pci_device->generic.expansion_rom = pci_read_config(pci_device,PCI_GENERIC_EXPANSION_ROM_OFFSET) & WORD_MASK;
                        pci_device->generic.int_line = pci_read_config(pci_device,PCI_GENERIC_INT_LINE_OFFSET) & BYTE_MASK;
                        pci_device->generic.max_latency = pci_read_config(pci_device,PCI_GENERIC_MAX_LATENCY_OFFSET) & BYTE_MASK;
                        pci_device->generic.min_grant = pci_read_config(pci_device,PCI_GENERIC_MIN_GRANT_OFFSET) & BYTE_MASK;
                        pci_device->generic.sub_device = pci_read_config(pci_device,PCI_GENERIC_SUB_DEVICE_OFFSET) & SHORT_MASK;
                        pci_device->generic.sub_vendor = pci_read_config(pci_device,PCI_GENERIC_SUB_VENDOR_OFFSET) & SHORT_MASK;
                        break;

                    default:
                        serial_printf("Unknown header type in PCI Device, skipping\n");
                        memset(pci_device,0,sizeof(struct pci_device));
                        continue;
                }

                if(print){
                    serial_printf("Found device on bus %i, device %i, function %i, vendor id %i device id %i class %s\n",bus,slot,function,pci_device->vendor_id,pci_device->device_id,
                                  pci_get_class_name(pci_device->class));
                }
                pci_device->pci_slot = p_slot;
                pci_device->registered = true;

            }
        }
    }
}



char* pci_get_class_name(uint8_t class) {

    switch (class){
        case 0x00: return "Unknown Device";
        case 0x01: return "Mass Storage Controller";
        case 0x02: return "Network Controller";
        case 0x03: return "Display Controller";
        case 0x04: return "Multimedia Controller";
        case 0x05: return "Memory Controller";
        case 0x06: return "Bridge";
        case 0x07: return "Simple Communication Controller";
        case 0x08: return "Base System Peripheral";
        case 0x09: return "Input Device Controller";
        case 0x0A: return "Docking Station";
        case 0x0B: return "Processor";
        case 0x0C: return "Serial Bus Controller";
        case 0x0D: return "Wireless Controller";
        case 0x0E: return "Intelligent Controller";
        case 0x0F: return "Satellite Communication Controller";
        case 0x10: return "Encryption Controller";
        case 0x11: return "Signal Processing Controller";
        case 0x12: return "Processing Accelerator";
        case 0x13: return "Non-Essential Instrumentation";
        case 0x40: return "Co-Processor";
        case 0xFF: return "Unassigned";

    }
    return "Unknown";
}