//
// Created by dustyn on 9/29/24.
//

#include "include/drivers/block/ahci.h"

uint32_t find_command_slot(struct ahci_device *device) {
    for (uint32_t i; i < device->parent->command_slots; i++) {
        if (((device->registers->sata_active | device->registers->command_issue) & (1 << i)) == 0) {
            return i;
        }
    }
    return UINT32_MAX;
}

struct ahci_command_table *set_prdt(struct ahci_command_header *header, uint64_t buffer, uint32_t interrupt_vector, uint32_t byte_count) {
    volatile struct ahci_command_table *command_table =
            P2V((uint64_t)((uint64_t) header->command_table_base_address | ((uint64_t) header->command_table_base_address_upper << 32)));

    command_table->prdt_entries[0].data_base_address = (uint32_t) buffer;
    command_table->prdt_entries[0].data_base_address_upper = (uint32_t)(buffer >> 32);
    command_table->prdt_entries[0].byte_count_and_interrupt_flag = byte_count | ((interrupt_vector & 1) << 31);

    return command_table;
}
