//
// Created by dustyn on 9/29/24.
//

#include "include/drivers/block/ahci.h"
#include "include/memory/mem.h"
#include "include/architecture/arch_timer.h"
#include "include/drivers/bus/pci.h"
#include "include/drivers/block/ahci.h"
#include "include/definitions/string.h"

struct ahci_controller controller = {0};
struct doubly_linked_list ahci_controller_list = {0};

static int32_t read_write_lba(struct ahci_device *device, char *buffer, uint64_t start, uint64_t count, bool write);

static int32_t ahci_probe(struct device *device);

struct block_device_ops ahci_block_ops = {
        .block_read = ahci_read_block,
        .block_write = ahci_write_block,
        .ahci_ops = NULL
};

struct pci_driver ahci_pci_driver = {
        .probe = NULL,
        .bind = NULL,
        .shutdown = NULL,
        .name = "ahcidriver",
        .driver_managed_dma = false,
        .resume = NULL,
        .device = NULL, // this struct can be copied for use later and then have fields reassigned
};

struct device_ops ahci_device_ops = {
        .block_device_ops = &ahci_block_ops,
        .shutdown = NULL,
        .reset = NULL,
        .get_status = NULL,
        .init = NULL,
        .configure = NULL,

};

struct device_driver ahci_driver = {
        .pci_driver = &ahci_pci_driver,
        .device_ops = &ahci_device_ops,
        .probe = ahci_probe,
};

uint32_t ahci_find_command_slot(struct ahci_device *device) {
    for (uint32_t i; i < device->parent->command_slots; i++) {
        if (((device->registers->sata_active | device->registers->command_issue) & (1 << i)) == 0) {
            return i;
        }
    }
    return UINT32_MAX;
}

uint64_t ahci_read_block(uint64_t block_number, size_t block_count, char *buffer, struct device *device) {
    return read_write_lba(device->device_info, buffer, block_number, block_count, false);
}

uint64_t ahci_write_block(uint64_t block_number, size_t block_count, char *buffer, struct device *device) {
    return read_write_lba(device->device_info, buffer, block_number, block_count, true);
}


volatile struct ahci_command_table *
set_prdt(volatile struct ahci_command_header *header, uint64_t buffer, uint32_t interrupt_vector, uint32_t byte_count) {
    volatile struct ahci_command_table *command_table =
    Phys2Virt((uint64_t)((uint64_t) header->command_table_base_address |
                                    ((uint64_t) header->command_table_base_address_upper << 32)));

    command_table->prdt_entries[0].data_base_address = (uint32_t) buffer;
    command_table->prdt_entries[0].data_base_address_upper = (uint32_t)(buffer >> 32);
    command_table->prdt_entries[0].byte_count_and_interrupt_flag = byte_count | ((interrupt_vector & 1) << 31);

    return command_table;
}

void ahci_send_command(uint32_t slot, struct ahci_device *device) {
    while ((device->registers->task_file_data & (0x88)) != 0) {}

    device->registers->command_and_status &= ~HBA_CMD_CR;

    while ((device->registers->command_and_status & HBA_CMD_ST) != 0) {}

    device->registers->command_and_status |= HBA_CMD_FR | HBA_CMD_ST;

    device->registers->command_issue = 1 << slot;

    while ((device->registers->command_issue & (1 << slot)) != 0) {}

    device->registers->command_and_status &= ~HBA_CMD_ST;

    while ((device->registers->command_and_status & HBA_CMD_ST) != 0) {}
    device->registers->command_and_status &= ~HBA_CMD_FR;
}

static int32_t read_write_lba(struct ahci_device *device, char *buffer, uint64_t start, uint64_t count, bool write) {

    uint32_t command_slot = ahci_find_command_slot(device);

    if (command_slot == UINT32_MAX) {
        return KERN_NO_RESOURCE;
    }

    volatile struct ahci_command_header *header =
    (struct ahci_command_header *) Phys2Virt(
            (((uint64_t) device->registers->command_list_base_address) |
    ((uint64_t) device->registers->command_list_base_address_upper << 32)) +
    ((uint64_t) command_slot * sizeof(struct ahci_command_header)));


    header->flags &= ~(0b11111 | (1 << 6));

    header->flags |= (uint16_t)(sizeof(struct ahci_fis_host_to_device) / 4);
    header->prdt_entry_count = 1;

    volatile struct ahci_command_table *table = set_prdt(header, (uint64_t)(Virt2Phys(buffer)), 1,
            (uint32_t)(count * SECTOR_SIZE - 1));

    volatile struct ahci_fis_host_to_device *command_pointer = (volatile struct ahci_fis_host_to_device *) &table->command_fis;
    memset((void *) command_pointer, 0, sizeof(struct ahci_fis_host_to_device));

    if (write) {
        command_pointer->command = WRITE;
    } else {
        command_pointer->command = READ;
    }

    command_pointer->fis_type = FIS_TYPE_REG_H2D;
    command_pointer->flags = (1 << 7);
    command_pointer->device = 1 << 6;
    command_pointer->lba_low = start & 0xFF;
    command_pointer->lba_mid = (start >> 8) & 0xFF;
    command_pointer->lba_high = (start >> 16) & 0xFF;
    command_pointer->lba_low_exp = (start >> 24) & 0xFF;
    command_pointer->lba_mid_exp = (start >> 32) & 0xFF;
    command_pointer->lba_high_exp = (start >> 40) & 0xFF;

    command_pointer->sector_count_low = count & 0xFF;
    command_pointer->sector_count_high = (count >> 8) & 0xFF;

    ahci_send_command(command_slot, device);

    return KERN_SUCCESS;
}

int32_t ahci_initialize_device(struct ahci_device *device) {
    uint32_t command_slot = ahci_find_command_slot(device);

    uint64_t
            command_list = (uint64_t)
    kmalloc(PAGE_SIZE);

    device->registers->command_list_base_address = (uint32_t) command_list;
    device->registers->command_list_base_address_upper = (uint32_t)(command_list >> 32);

    for (int i = 0; i < 32; i++) {
        volatile struct ahci_command_header *header = Phys2Virt(
                (((uint64_t) device->registers->command_list_base_address) |
        (uint64_t) device->registers->command_list_base_address_upper
                << 32) + ((i * sizeof(struct ahci_command_header))));
        uint64_t
                descriptor_base = (uint64_t)
        kmalloc(PAGE_SIZE);
        header->command_table_base_address = (uint32_t) descriptor_base;
        header->command_table_base_address_upper = (uint32_t)(descriptor_base >> 32);
    }

    uint64_t
            fis_base = (uint64_t)
    kmalloc(PAGE_SIZE);
    device->registers->fis_base_address = (uint32_t) fis_base;
    device->registers->fis_base_address_upper = (uint32_t)(fis_base >> 32);
    device->registers->command_and_status |= (1 << 0) | (1 << 4);

    volatile struct ahci_command_header *header = Phys2Virt((((uint64_t) device->registers->command_list_base_address) |
                                                            (uint64_t) device->registers->command_list_base_address_upper
                                                                    << 32) +
                                                            ((command_slot * sizeof(struct ahci_command_header))));

    header->flags &= ~0b11111 | (1 << 7);
    header->flags |= (uint16_t)(sizeof(struct ahci_fis_host_to_device) / 4);
    header->prdt_entry_count = 1;

    uint16_t *identity = kmalloc(PAGE_SIZE);

    volatile struct ahci_command_table *table = set_prdt(header, (uint64_t)Virt2Phys((uint64_t) identity), 1, 511);

    volatile struct ahci_fis_host_to_device *command = (struct ahci_fis_host_to_device *) table->command_fis;
    memset((void *) command, 0, sizeof(struct ahci_fis_host_to_device));

    command->command = 0xEC;
    command->flags = (1 << 7);
    command->fis_type = FIS_TYPE_REG_H2D;

    ahci_send_command(command_slot, device);

    uint64_t *sector_count = (uint64_t * ) & identity[100];

    char *serial_number = kmalloc(21);
    char *firmware_revision = kmalloc(9);
    char *model_number = kmalloc(48);

    memcpy(serial_number, (char *) identity + 20, 20);
    to_big_endian(serial_number, 20);

    memcpy(firmware_revision, (char *) identity + 46, 8);
    to_big_endian(firmware_revision, 8);

    memcpy(model_number, (char *) identity + 54, 40);
    to_big_endian(model_number, 40);

    info_printf("AHCI Device Initialized: Serial Number %s , Firmware Revision %s , Model No %s , Sector Count %i\n",
                serial_number, firmware_revision, model_number, sector_count);

    return KERN_SUCCESS;

}

int32_t ahci_give_kernel_ownership(struct ahci_controller *controller) {

    if ((controller->ahci_regs->capabilities_extended & (1 << 0)) == 0) {
        err_printf("AHCI: Bios Handoff Not Supported\n");
        return KERN_NOT_SUPPORTED;
    }

    controller->ahci_regs->bios_os_handoff_control_status |= (1 << 1);

    while ((controller->ahci_regs->bios_os_handoff_control_status & (1 << 0)) == 0) {
        __asm__ __volatile__("pause");
    }

    timer_sleep(25);

    if (controller->ahci_regs->bios_os_handoff_control_status & (1 << 4)) {
        timer_sleep(2000);
    }

    if ((controller->ahci_regs->bios_os_handoff_control_status & (1 << 4)) ||
        (controller->ahci_regs->bios_os_handoff_control_status & (1 << 0)) ||
        ((controller->ahci_regs->bios_os_handoff_control_status & (1 << 1)) == 0)) {
        err_printf("AHCI: Bios Handoff Failed\n");
        return KERN_IO_ERROR;
    }

    return KERN_SUCCESS;
}

static bool is_bar_64_bit(uint32_t bar) {
    if ((bar & 1) != 0) {
        return false;
    }
    uint32_t type = (bar >> 1) & 3;
    return type == 2;
}

int32_t initialize_ahci_controller(struct device *device) {
    struct ahci_controller *controller = device->device_info;
    struct pci_device *pci_dev = device->pci_device;

    controller->ahci_regs = Phys2Virt(pci_dev->generic.base_address_registers[5]);

    controller->major = (controller->ahci_regs->version >> 16) & SHORT_MASK;
    controller->minor = controller->ahci_regs->version & SHORT_MASK;

    if ((controller->ahci_regs->capabilities & (1 << 31)) == 0) {
        warn_printf("AHCI: Controller does not support 64 bit addressing");
        return KERN_NOT_SUPPORTED;
    }

    ahci_give_kernel_ownership(controller);

    controller->ahci_regs->global_host_control |= (1 << 31);

    controller->ahci_regs->global_host_control &= ~(1 << 1);

    controller->port_count = controller->ahci_regs->capabilities & 0x1F;
    controller->command_slots = (controller->ahci_regs->capabilities >> 8) & 0x1F;

    for (size_t i = 0; i < controller->port_count; i++) {
        if (controller->ahci_regs->ports_implemented & (1 << i) != 0) {
            volatile struct ahci_port_registers *port =
                    (volatile struct ahci_port_registers *) controller->pci_bar + sizeof(struct ahci_registers) +
                    (i * sizeof(struct ahci_port_registers));

            port = Phys2Virt(port);

            if (port->signature == SATA_ATA) {
                struct ahci_device *ahci_device = kmalloc(sizeof(struct ahci_device));
                ahci_device->controller = device;
                ahci_device->registers = port;
                ahci_device->parent = controller;

                int32_t ret = ahci_initialize_device(ahci_device);

                //it can only return success for now but this will be important if that changes
                if (ret != KERN_SUCCESS) {
                    kfree(ahci_device);
                    warn_printf("Unable to set up AHCI device, moving on..\n");
                    continue;
                }


                struct device *ahci_dev = kmalloc(sizeof(struct device));
                device->device_info = ahci_device;
                device->driver = &ahci_driver;


                insert_device_into_kernel_tree(ahci_dev);


            }


        }
    }


}


void setup_ahci_device(struct pci_device *pci_device) {
    static uint32_t controller_count = 0;
    pci_enable_bus_mastering(pci_device);
    struct device *ahci_controller = kzmalloc(sizeof(struct device));
    struct ahci_controller *controller = kzmalloc(sizeof(struct ahci_controller));
    ahci_controller->driver = &ahci_driver;
    ahci_controller->pci_device = pci_device;
    ahci_controller->device_info = controller;
    ahci_controller->device_major = DEVICE_MAJOR_AHCI;
    ahci_controller->device_minor = device_minor_map[DEVICE_MAJOR_AHCI]++;
    ahci_controller->device_class = AHCI_CLASS;
    ahci_controller->lock = kmalloc(sizeof(struct spinlock));
    ahci_controller->device_type = DEVICE_TYPE_BLOCK;
    ahci_controller->pci_device = pci_device;
    ksprintf(ahci_controller->name, "ahcictlr%i", controller_count++);

    int32_t ret = ahci_controller->driver->probe(ahci_controller);

    if (ret != KERN_SUCCESS) {
        warn_printf("AHCI Controller Setup Failed.\n");
        kfree(ahci_controller);
        kfree(controller);
        return;
    }

    insert_device_into_kernel_tree(ahci_controller);
}


static void ahci_bind(struct device *device) {
    static int32_t ahci_number = 0;
    char name[32];
    ksprintf(name, "ahcidevice_%i", ahci_number++);
    safe_strcpy(device->name, name, 30);
    device->name[31] = '\0'; // call it paranoia but im doing this anyway
}

static int32_t ahci_probe(struct device *device) {
    struct pci_device *pci_device = device->pci_device;
    if (!pci_device) {
        return KERN_NOT_FOUND;
    }

    return initialize_ahci_controller(device);
}