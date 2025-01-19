//
// Created by dustyn on 9/17/24.
//

#include "include/drivers/block/nvme.h"
#include "include/device/device.h"

static uint64_t nvme_read_raw_block(struct device *dev, uint64_t block_number, uint64_t block_count, void *buffer){
    struct nvme_device *nvme_dev = dev->device_info;
    struct nvme_command command;
    uint32_t status;



}


static uint64_t nvme_write_raw_block(struct device *dev, uint64_t block_number, uint64_t block_count, void *buffer){

}