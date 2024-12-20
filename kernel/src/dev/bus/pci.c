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

struct doubly_linked_list pci_device_list;
uintptr_t pci_mmio_address = 0;
uint8_t start_bus = 0;
uint8_t end_bus = 0;


static uint32_t pci_read_config(struct pci_device *device,uint16_t offset) {
    uintptr_t address = (uintptr_t) P2V(pci_mmio_address)
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
void set_pci_mmio_address(struct mcfg_entry *entry){
    pci_mmio_address = entry->base_address;
    start_bus = entry->start_bus;
    end_bus = entry->end_bus;
    serial_printf("PCI MMIO address is %x.64 , start bus %i end bus %i\n",pci_mmio_address,start_bus,end_bus);
}


void pci_scan(bool print){
    for(uint16_t bus = 0; bus < PCI_MAX_BUSES; bus++){
        for(uint8_t device = 0; device < SLOTS_PER_BUS; device++){
            for(uint8_t function = 0; function < FUNCTIONS_PER_DEVICE; function++){

                struct pci_device *pci_device = kmalloc(sizeof(struct pci_device));
                uint16_t vendor_id = pci_read_config(pci_device,PCI_DEVICE_VENDOR_ID_OFFSET) & SHORT_MASK;

                if(vendor_id == PCI_DEVICE_DOESNT_EXIST){
                    if(function == 0){
                        kfree(pci_device);
                        break;
                    }
                    kfree(pci_device);
                    continue;
                }

                pci_device->bus = bus;
                pci_device->slot = device;
                pci_device->function = function;

                uint8_t class = pci_read_config(pci_device,PCI_DEVICE_CLASS_OFFSET) & BYTE_MASK;

                if(class == PCI_CLASS_UNASSIGNED || strcmp(pci_get_class_name(class),"Unknown")){
                    kfree(pci_device);
                    continue;
                }

                pci_device->bus = bus;
                pci_device->slot = device;
                pci_device->function = function;
                pci_device->vendor_id = vendor_id;
                pci_device->device_id = pci_read_config(pci_device,PCI_DEVICE_ID_OFFSET) & SHORT_MASK;
                pci_device->class = class;

                if(print){
                    serial_printf("Found device on bus %i, device %i, function %i, vendor id %i device id %i class %s\n",bus,device,function,pci_device->vendor_id,pci_device->device_id,
                                  pci_get_class_name(pci_device->class));
                }


                pci_insert_device_into_list(pci_device);

            }
        }
    }
}


char* pci_get_class_name(uint8_t class) {

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