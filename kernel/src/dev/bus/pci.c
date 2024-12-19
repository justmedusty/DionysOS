//
// Created by dustyn on 12/19/24.
//

#include "include/dev/bus/pci.h"
#include"include/data_structures/doubly_linked_list.h"
#include "include/definitions.h"
#include "include/mem/kalloc.h"
#include "include/drivers/serial/uart.h"

struct doubly_linked_list pci_device_list;
uintptr_t pci_mmio_address = 0;


static uint32_t pci_read_config(struct pci_device *device,uint16_t offset) {
    uintptr_t address = (uintptr_t) pci_mmio_address
                        + (uintptr_t) (device->bus << BUS_SHIFT)
                        + (uintptr_t) (device->slot << DEVICE_SHIFT)
                        + (uintptr_t) (device->function << FUNCTION_SHIFT)
                        + (uintptr_t) offset;

    return *(volatile uint32_t *) address;
}

static void pci_insert_device_into_list(struct pci_device *device){
    doubly_linked_list_insert_tail(&pci_device_list,device);
}

void pci_init(){
    doubly_linked_list_init(&pci_device_list);

}
void set_pci_mmio_address(uintptr_t address){
    pci_mmio_address = address;
}


void pci_scan(bool print){
    for(uint16_t bus = 0; bus < PCI_MAX_BUSES; bus++){
        for(uint8_t device = 0; device < SLOTS_PER_BUS; device++){
            for(uint8_t function = 0; function < FUNCTIONS_PER_DEVICE; function++){
                struct pci_device pci_device = {
                        .bus = (uint8_t) bus,
                        .slot = device,
                        .function = function,
                };

                uint16_t vendor_id = pci_read_config(&pci_device,PCI_DEVICE_VENDOR_ID_OFFSET) & PCI_ID_MASK;

                if(vendor_id == PCI_DEVICE_DOESNT_EXIST){
                    if(function == 0){
                        break;
                    }
                    continue;
                }
                uint16_t device_id = pci_read_config(&pci_device,PCI_DEVICE_ID_OFFSET);

                if(print){
                    serial_printf("Found device on bus %i, device %i, function %i, vendor id %i device id %i\n",bus,device,function,vendor_id,device_id);
                }

                pci_device.vendor_id = vendor_id;
                pci_device.device_id = device_id;

                struct pci_device *new_device = kmalloc(sizeof(struct pci_device));

                *new_device = pci_device;

            }
        }
    }
}


char* pci_get_class_name(uint8_t class){

    switch (class){

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