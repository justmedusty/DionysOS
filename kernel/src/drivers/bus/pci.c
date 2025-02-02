//
// Created by dustyn on 12/19/24.
//

#include "include/drivers/bus/pci.h"
#include "include/data_structures/binary_tree.h"
#include "include/drivers/display/framebuffer.h"
#include"include/data_structures/doubly_linked_list.h"
#include "include/definitions/definitions.h"
#include "include/memory/kalloc.h"
#include "include/drivers/serial/uart.h"
#include "include/definitions/string.h"
#include "include/memory/mem.h"
#include "include/drivers/block/nvme.h"

/*
 * PCIe Functionality , the MMIO address is obtained via the MCFG table via an acpi table lookup
 */


struct pci_bus pci_buses[PCI_MAX_BUSES] = {0};

struct doubly_linked_list registered_pci_devices;

struct pci_bus_information pci_info = {
        .pci_mmio_address = 0,
        .start_bus = 0,
        .end_bus = 0
};


/*
 * PCIe still uses the oldschool bus:slot:function layout so we use the those values + the shift macros in order to get information
 */
static uint32_t pci_read_config(struct pci_device *device, uint16_t offset) {
    uintptr_t address = (uintptr_t) P2V(pci_info.pci_mmio_address)
                        + (uintptr_t) (device->bus << BUS_SHIFT)
                        + (uintptr_t) (device->slot << SLOT_SHIFT)
                        + (uintptr_t) (device->function << FUNCTION_SHIFT)
                        + (uintptr_t) offset;

    return *(volatile uint32_t *) address;
}

static void pci_write_config(struct pci_device *device, uint16_t offset, uint32_t value) {
    uintptr_t address = (uintptr_t) P2V(pci_info.pci_mmio_address)
                        + (uintptr_t) (device->bus << BUS_SHIFT)
                        + (uintptr_t) (device->slot << SLOT_SHIFT)
                        + (uintptr_t) (device->function << FUNCTION_SHIFT)
                        + (uintptr_t) offset;

    *(volatile uint32_t *) address = value;
}

#ifdef __x86_64__

#include "include/architecture/x86_64/acpi.h"
#include "include/drivers/block/nvme.h"

void set_pci_mmio_address(struct mcfg_entry *entry) {
    doubly_linked_list_init(&registered_pci_devices);
    pci_info.pci_mmio_address = entry->base_address;
    pci_info.start_bus = entry->start_bus;
    pci_info.end_bus = entry->end_bus;
    serial_printf("PCIe MMIO address is %x.64 , start bus %i end bus %i\n", pci_info.pci_mmio_address,
                  pci_info.start_bus, pci_info.end_bus);
}

#endif

struct pci_bus_information get_pci_info() {
    return pci_info;
}

void pci_enumerate_devices(bool print) {
    for (uint16_t bus = 0; bus < PCI_MAX_BUSES; bus++) {
        for (uint8_t slot = 0; slot < SLOTS_PER_BUS; slot++) {
            for (uint8_t function = 0; function < FUNCTIONS_PER_DEVICE; function++) {

                struct pci_bus *p_bus = &pci_buses[bus];
                struct pci_slot *p_slot = &p_bus->pci_slots[slot];
                struct pci_device *pci_device = &p_slot->pci_devices[function];

                if (pci_device->registered) {
                    continue;
                }

                pci_device->bus = bus;
                pci_device->slot = slot;
                pci_device->function = function;

                uint16_t vendor_id = pci_read_config(pci_device, PCI_DEVICE_VENDOR_ID_OFFSET) & SHORT_MASK;

                if (vendor_id == PCI_DEVICE_DOESNT_EXIST) {
                    if (function == 0) {
                        memset(pci_device, 0, sizeof(struct pci_device));
                        break;
                    }
                    memset(pci_device, 0, sizeof(struct pci_device));
                    continue;
                }


                uint8_t class = pci_read_config(pci_device, PCI_DEVICE_CLASS_OFFSET) & BYTE_MASK;

                if (class == PCI_CLASS_UNASSIGNED || safe_strcmp(pci_get_class_name(class), "Unknown", 100)) {
                    memset(pci_device, 0, sizeof(struct pci_device));
                    continue;
                }

                pci_device->bus = bus;
                pci_device->slot = slot;
                pci_device->function = function;
                pci_device->vendor_id = vendor_id;
                pci_device->class = class;
                pci_device->device_id = pci_read_config(pci_device, PCI_DEVICE_ID_OFFSET) & SHORT_MASK;
                pci_device->command = pci_read_config(pci_device, PCI_DEVICE_COMMAND_OFFSET) & SHORT_MASK;
                pci_device->revision_id = pci_read_config(pci_device, PCI_DEVICE_REVISION_OFFSET) & SHORT_MASK;
                pci_device->header_type = pci_read_config(pci_device, PCI_DEVICE_HEADER_TYPE_OFFSET) & BYTE_MASK;
                pci_device->class = class;

                switch (pci_device->header_type) {
                    case PCI_TYPE_BRIDGE:
                        pci_device->bridge.base_address_registers[0] =
                                pci_read_config(pci_device, PCI_BRIDGE_BAR0_OFFSET) & WORD_MASK;
                        pci_device->bridge.base_address_registers[1] =
                                pci_read_config(pci_device, PCI_BRIDGE_BAR1_OFFSET) & WORD_MASK;
                        pci_device->bridge.bus_primary =
                                pci_read_config(pci_device, PCI_BRIDGE_PRIMARY_BUS_OFFSET) & BYTE_MASK;
                        pci_device->bridge.bus_secondary =
                                pci_read_config(pci_device, PCI_BRIDGE_SECONDARY_BUS_OFFSET) & BYTE_MASK;
                        pci_device->bridge.bus_subordinate =
                                pci_read_config(pci_device, PCI_BRIDGE_SUBORDINATE_BUS_OFFSET) & BYTE_MASK;
                        pci_device->bridge.latency_timer2 =
                                pci_read_config(pci_device, PCI_BRIDGE_LATENCY_TIMER_OFFSET) & BYTE_MASK;
                        pci_device->bridge.io_base = pci_read_config(pci_device, PCI_BRIDGE_IO_BASE_OFFSET) & BYTE_MASK;
                        pci_device->bridge.io_limit =
                                pci_read_config(pci_device, PCI_BRIDGE_IO_LIMIT_OFFSET) & BYTE_MASK;
                        pci_device->bridge.status2 =
                                pci_read_config(pci_device, PCI_BRIDGE_STATUS2_OFFSET) & SHORT_MASK;
                        pci_device->bridge.mem_base =
                                pci_read_config(pci_device, PCI_BRIDGE_MEM_BASE_OFFSET) & SHORT_MASK;
                        pci_device->bridge.mem_limit =
                                pci_read_config(pci_device, PCI_BRIDGE_MEM_LIMIT_OFFSET) & SHORT_MASK;
                        pci_device->bridge.pre_base =
                                pci_read_config(pci_device, PCI_BRIDGE_PREFETCH_BASE_OFFSET) & SHORT_MASK;
                        pci_device->bridge.pre_limit =
                                pci_read_config(pci_device, PCI_BRIDGE_PREFETCH_LIMIT_OFFSET) & SHORT_MASK;
                        pci_device->bridge.pre_base_upper =
                                pci_read_config(pci_device, PCI_BRIDGE_PREFETCH_BASE_UPPER_OFFSET) & WORD_MASK;
                        pci_device->bridge.pre_limit_upper =
                                pci_read_config(pci_device, PCI_BRIDGE_PREFETCH_LIMIT_UPPER_OFFSET) & WORD_MASK;
                        pci_device->bridge.io_base_upper =
                                pci_read_config(pci_device, PCI_BRIDGE_IO_BASE_UPPER_OFFSET) & SHORT_MASK;
                        pci_device->bridge.io_limit_upper =
                                pci_read_config(pci_device, PCI_BRIDGE_IO_LIMIT_UPPER_OFFSET) & SHORT_MASK;
                        pci_device->bridge.capabilities =
                                pci_read_config(pci_device, PCI_BRIDGE_CAPABILITIES_OFFSET) & BYTE_MASK;
                        pci_device->bridge.expansion_rom =
                                pci_read_config(pci_device, PCI_BRIDGE_EXPANSION_ROM_OFFSET) & WORD_MASK;
                        pci_device->bridge.int_line =
                                pci_read_config(pci_device, PCI_BRIDGE_INT_LINE_OFFSET) & BYTE_MASK;
                        pci_device->bridge.int_pin = pci_read_config(pci_device, PCI_BRIDGE_INT_PIN_OFFSET) & BYTE_MASK;
                        pci_device->bridge.bridge_control =
                                pci_read_config(pci_device, PCI_BRIDGE_CONTROL_OFFSET) & SHORT_MASK;
                        break;
                    case PCI_TYPE_GENERIC_DEVICE:
                        pci_device->generic.base_address_registers[0] =
                                pci_read_config(pci_device, PCI_GENERIC_BAR0_OFFSET) & WORD_MASK;
                        pci_device->generic.base_address_registers[1] =
                                pci_read_config(pci_device, PCI_GENERIC_BAR1_OFFSET) & WORD_MASK;
                        pci_device->generic.base_address_registers[2] =
                                pci_read_config(pci_device, PCI_GENERIC_BAR2_OFFSET) & WORD_MASK;
                        pci_device->generic.base_address_registers[3] =
                                pci_read_config(pci_device, PCI_GENERIC_BAR3_OFFSET) & WORD_MASK;
                        pci_device->generic.base_address_registers[4] =
                                pci_read_config(pci_device, PCI_GENERIC_BAR4_OFFSET) & WORD_MASK;
                        pci_device->generic.base_address_registers[5] =
                                pci_read_config(pci_device, PCI_GENERIC_BAR5_OFFSET) & WORD_MASK;
                        pci_device->generic.capabilities =
                                pci_read_config(pci_device, PCI_GENERIC_CAPABILITIES_OFFSET) & BYTE_MASK;
                        pci_device->generic.expansion_rom =
                                pci_read_config(pci_device, PCI_GENERIC_EXPANSION_ROM_OFFSET) & WORD_MASK;
                        pci_device->generic.int_line =
                                pci_read_config(pci_device, PCI_GENERIC_INT_LINE_OFFSET) & BYTE_MASK;
                        pci_device->generic.max_latency =
                                pci_read_config(pci_device, PCI_GENERIC_MAX_LATENCY_OFFSET) & BYTE_MASK;
                        pci_device->generic.min_grant =
                                pci_read_config(pci_device, PCI_GENERIC_MIN_GRANT_OFFSET) & BYTE_MASK;
                        pci_device->generic.sub_device =
                                pci_read_config(pci_device, PCI_GENERIC_SUB_DEVICE_OFFSET) & SHORT_MASK;
                        pci_device->generic.sub_vendor =
                                pci_read_config(pci_device, PCI_GENERIC_SUB_VENDOR_OFFSET) & SHORT_MASK;
                        break;

                    default:
                        serial_printf("Unknown header type in PCI Device, skipping\n");
                        memset(pci_device, 0, sizeof(struct pci_device));
                        continue;
                }

                if (print) {
                    serial_printf(
                            "Found device on bus %i, device %i, function %i, vendor id %i device id %i type %s\n", bus,
                            slot, function, pci_device->vendor_id, pci_device->device_id,
                            pci_get_subclass_name(pci_device->class, pci_device->subclass));
                }
                pci_device->pci_slot = p_slot;
                pci_device->registered = true;
                if (IS_NVME_CONTROLLER(pci_device)) {
                    setup_nvme_device(
                            pci_device);
                }
                doubly_linked_list_insert_head(&registered_pci_devices, pci_device);
                info_printf("PCI device of type %s inserted into registered device list\n",
                            pci_get_subclass_name(pci_device->class, pci_device->subclass));

            }
        }
    }
}


char *pci_get_class_name(uint8_t class) {

    switch (class) {
        case 0x00:
            return "Unknown Device";
        case 0x01:
            return "Mass Storage Controller";
        case 0x02:
            return "Network Controller";
        case 0x03:
            return "Display Controller";
        case 0x04:
            return "Multimedia Controller";
        case 0x05:
            return "Memory Controller";
        case 0x06:
            return "Bridge";
        case 0x07:
            return "Simple Communication Controller";
        case 0x08:
            return "Base System Peripheral";
        case 0x09:
            return "Input Device Controller";
        case 0x0A:
            return "Docking Station";
        case 0x0B:
            return "Processor";
        case 0x0C:
            return "Serial Bus Controller";
        case 0x0D:
            return "Wireless Controller";
        case 0x0E:
            return "Intelligent Controller";
        case 0x0F:
            return "Satellite Communication Controller";
        case 0x10:
            return "Encryption Controller";
        case 0x11:
            return "Signal Processing Controller";
        case 0x12:
            return "Processing Accelerator";
        case 0x13:
            return "Non-Essential Instrumentation";
        case 0x40:
            return "Co-Processor";
        case 0xFF:
            return "Unassigned";

        default:
            return "Unknown";

    }
}

char *pci_get_subclass_name(uint8_t class, uint8_t subclass) {
    switch (class) {
        case 0x01: // Mass Storage Controller
            switch (subclass) {
                case 0x00:
                    return "SCSI Bus Controller";
                case 0x01:
                    return "IDE Controller";
                case 0x02:
                    return "Floppy Disk Controller";
                case 0x03:
                    return "IPI Bus Controller";
                case 0x04:
                    return "RAID Controller";
                case 0x05:
                    return "ATA Controller";
                case 0x06:
                    return "Serial ATA Controller";
                case 0x07:
                    return "Serial Attached SCSI Controller";
                case 0x08:
                    return "Non-Volatile Memory Controller";
                case 0x80:
                    return "Other Mass Storage Controller";
                default:
                    return "Unknown Mass Storage Subclass";
            }

        case 0x02: // Network Controller
            switch (subclass) {
                case 0x00:
                    return "Ethernet Controller";
                case 0x01:
                    return "Token Ring Controller";
                case 0x02:
                    return "FDDI Controller";
                case 0x03:
                    return "ATM Controller";
                case 0x04:
                    return "ISDN Controller";
                case 0x05:
                    return "WorldFip Controller";
                case 0x06:
                    return "PICMG Controller";
                case 0x80:
                    return "Other Network Controller";
                default:
                    return "Unknown Network Subclass";
            }

        case 0x03: // Display Controller
            switch (subclass) {
                case 0x00:
                    return "VGA Compatible Controller";
                case 0x01:
                    return "XGA Controller";
                case 0x02:
                    return "3D Controller";
                case 0x80:
                    return "Other Display Controller";
                default:
                    return "Unknown Display Subclass";
            }

        case 0x04: // Multimedia Controller
            switch (subclass) {
                case 0x00:
                    return "Multimedia Video Controller";
                case 0x01:
                    return "Multimedia Audio Controller";
                case 0x02:
                    return "Computer Telephony Device";
                case 0x03:
                    return "Audio Device";
                case 0x80:
                    return "Other Multimedia Controller";
                default:
                    return "Unknown Multimedia Subclass";
            }

        case 0x06: // Bridge Device
            switch (subclass) {
                case 0x00:
                    return "Host Bridge";
                case 0x01:
                    return "ISA Bridge";
                case 0x02:
                    return "EISA Bridge";
                case 0x03:
                    return "MCA Bridge";
                case 0x04:
                    return "PCI-to-PCI Bridge";
                case 0x05:
                    return "PCMCIA Bridge";
                case 0x06:
                    return "NuBus Bridge";
                case 0x07:
                    return "CardBus Bridge";
                case 0x08:
                    return "RACEway Bridge";
                case 0x09:
                    return "PCI-to-PCI Bridge (Secondary)";
                case 0x0A:
                    return "InfiniBand-to-PCI Host Bridge";
                case 0x80:
                    return "Other Bridge Device";
                default:
                    return "Unknown Bridge Subclass";
            }

        case 0x0C: // Serial Bus Controller
            switch (subclass) {
                case 0x00:
                    return "FireWire (IEEE 1394) Controller";
                case 0x01:
                    return "ACCESS Bus";
                case 0x02:
                    return "SSA";
                case 0x03:
                    return "USB Controller";
                case 0x04:
                    return "Fibre Channel";
                case 0x05:
                    return "SMBus Controller";
                case 0x06:
                    return "InfiniBand Controller";
                case 0x07:
                    return "IPMI Interface";
                case 0x08:
                    return "SERCOS Interface";
                case 0x09:
                    return "CANbus Controller";
                case 0x80:
                    return "Other Serial Bus Controller";
                default:
                    return "Unknown Serial Bus Subclass";
            }

        case 0x0D: // Wireless Controller
            switch (subclass) {
                case 0x00:
                    return "iRDA Controller";
                case 0x01:
                    return "Consumer IR Controller";
                case 0x10:
                    return "RF Controller";
                case 0x11:
                    return "Bluetooth Controller";
                case 0x12:
                    return "Broadband Controller";
                case 0x20:
                    return "Ethernet Controller (802.1a)";
                case 0x21:
                    return "Ethernet Controller (802.1b)";
                case 0x80:
                    return "Other Wireless Controller";
                default:
                    return "Unknown Wireless Subclass";
            }

        default:
            return "Unknown Subclass";
    }
}

/*
 * This is not very helpful if there is multiple and you want a specific device but that is fine for now
 */
struct pci_device *get_pci_device_from_subclass(uint8_t class, uint8_t subclass) {
    acquire_spinlock(&registered_pci_devices.lock);
    struct doubly_linked_list_node *current = registered_pci_devices.head;

    while (current) {
        struct pci_device *dev = current->data;
        if (dev->class == class && dev->subclass == subclass) {
            return dev;
        }
        current = current->next;
    }
    return NULL;
}

uint32_t pci_read_base_address_register(struct device *device, int32_t base_address_register_number) {
    uint32_t bar = pci_info.pci_mmio_address + base_address_register_number * 4;

    uint32_t address = pci_read_config(device->pci_device, bar) & WORD_MASK;

    if (address == WORD_MASK) {
        return address;
    } else if (address & PCI_BASE_ADDRESS_SPACE_IO) {
        return address & PCI_BASE_ADDRESS_IO_MASK;
    } else {
        return address & PCI_BASE_ADDRESS_MEM_MASK;
    }
}

void pci_write_base_address_register(struct device *device, int32_t base_address_register_number, uint32_t address) {
    uint32_t base_address_register = pci_info.pci_mmio_address + base_address_register_number * 4;
    pci_write_config(device->pci_device, base_address_register, address);
}