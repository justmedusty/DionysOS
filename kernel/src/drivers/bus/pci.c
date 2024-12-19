//
// Created by dustyn on 12/19/24.
//

#include "pci.h"
#include"include/data_structures/doubly_linked_list.h"
#include "include/definitions.h"
#include "include/mem/kalloc.h"

void *pci_mmio_address = 0;
struct doubly_linked_list pci_device_list;

static uint32_t pci_read_config(struct pci_device *device) {
    uintptr_t address = (uintptr_t) pci_mmio_address
                        + (uintptr_t) (device->bus << BUS_SHIFT)
                        + (uintptr_t) (device->device << DEVICE_SHIFT)
                        + (uintptr_t) (device->function << FUNCTION_SHIFT)
                        + (uintptr_t) device->offset;

    return *(volatile uint32_t *) address;
}

static void pci_insert_device_into_list(struct pci_device *device){
    doubly_linked_list_insert_tail(&pci_device_list,device);
}
void pci_init(){
   // doubly_linked_list_init()
}

void pci_scan(){
    for(uint16_t bus = 0; bus < PCI_MAX_BUSES; bus++){
        for(uint8_t device = 0; device < DEVICES_PER_BUS; device++){
            for(uint8_t function = 0; function < FUNCTIONS_PER_DEVICE; function++){
                struct pci_device pci_device = {
                        .bus = (uint8_t) bus,
                        .device = device,
                        .function = function,
                        .offset = PCI_DEVICE_VENDOR_ID_OFFSET
                };

                uint16_t vendor_id = pci_read_config(&pci_device) & PCI_DEVICE_DOESNT_EXIST;

                if(vendor_id == PCI_DEVICE_DOESNT_EXIST){
                    if(function == 0){
                        break;
                    }
                    continue;
                }

                struct pci_device *new_device = kmalloc(sizeof(struct pci_device));

                *new_device = pci_device;



            }
        }
    }
}

