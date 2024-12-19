//
// Created by dustyn on 12/19/24.
//

#ifndef KERNEL_PCI_H
#define KERNEL_PCI_H

#include <stdint.h>

#define PCI_MAX_BUSES 256
#define DEVICES_PER_BUS 32
#define FUNCTIONS_PER_DEVICE 8
#define CONFIG_SPACE_PER_DEVICE 4096

#define PCI_DEVICE_VENDOR_ID_OFFSET 0x00
#define PCI_DEVICE_ID_OFFSET 0x02
#define PCI_DEVICE_TYPE_OFFSET 0x0E

#define PCI_DEVICE_DOESNT_EXIST 0xFFFF

#define BUS_SHIFT 20
#define DEVICE_SHIFT 15
#define FUNCTION_SHIFT 12

struct pci_device {
    uint16_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t offset;
};


#endif //KERNEL_PCI_H
