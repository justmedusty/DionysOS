//
// Created by dustyn on 9/29/24.
//

#include "include/drivers/block/ahci.h"
#include "include/memory/mem.h"

uint32_t ahci_find_command_slot(struct ahci_device *device) {
    for (uint32_t i; i < device->parent->command_slots; i++) {
        if (((device->registers->sata_active | device->registers->command_issue) & (1 << i)) == 0) {
            return i;
        }
    }
    return UINT32_MAX;
}

struct ahci_command_table *
set_prdt(volatile struct ahci_command_header *header, uint64_t buffer, uint32_t interrupt_vector, uint32_t byte_count) {
    volatile struct ahci_command_table *command_table =
            P2V((uint64_t)((uint64_t) header->command_table_base_address |
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

int32_t read_write_lba(struct ahci_device *device, char *buffer, uint64_t start, uint64_t count, bool write) {

    uint32_t command_slot = ahci_find_command_slot(device);

    if (command_slot == UINT32_MAX) {
        return KERN_NO_RESOURCE;
    }

    volatile struct ahci_command_header *header = (struct ahci_command_header *) P2V(
            (((uint64_t) device->registers->command_list_base_address) |
             ((uint64_t) device->registers->command_list_base_address_upper << 32)) +
            ((uint64_t) command_slot * sizeof(struct ahci_command_header)));


    header->flags &= ~(0b11111 | (1 << 6));

    header->flags |= (uint16_t)(sizeof(struct ahci_fis_host_to_device) / 4);
    header->prdt_entry_count = 1;

    volatile struct ahci_command_table *table = set_prdt(header, (uint64_t)(V2P(buffer)), 1,
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

    return KERN_SUCCESS;
}
