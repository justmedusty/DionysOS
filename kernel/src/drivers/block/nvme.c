//
// Created by dustyn on 9/17/24.
//

#include "include/drivers/block/nvme.h"
#include "include/device/device.h"

static uint64_t nvme_read_raw_block(struct device *dev, uint64_t block_number, uint64_t block_count, void *buffer) {
    struct nvme_device *nvme_dev = dev->device_info;
    struct nvme_command command;
    uint32_t status;


}


static uint64_t nvme_write_raw_block(struct device *dev, uint64_t block_number, uint64_t block_count, void *buffer) {

}

static int32_t nvme_configure_admin_queue(struct nvme_device *nvme_dev) {

}

static int32_t
nvme_submit_admin_command(struct nvme_device *nvme_device, struct nvme_command *command, uint32_t *result) {

}


static void nvme_submit_command(struct nvme_queue *queue, struct nvme_command *command) {

}

static int32_t nvme_wait_ready(struct nvme_device *nvme_device, bool enabled) {

}


static int32_t
nvme_setup_physical_region_pools(struct nvme_device *nvme_dev, uint64_t *physical_region_pool, int32_t total_length,
                                 uint64_t dma_address) {

}

static uint16_t nvme_read_completion_status(struct nvme_queue *queue, uint16_t index) {

}

static int32_t
nvme_submit_sync_command(struct nvme_queue *queue, struct nvme_command *command, uint32_t result, uint64_t timeout) {


}

static struct nvme_queue *nvme_alloc_queue(struct nvme_device *device, int32_t queue_id, int32_t depth) {

}

static int32_t nvme_get_info_from_identify(struct nvme_device *device) {

}

static int32_t nvme_setup_io_queues(struct nvme_device *device) {

}


static int32_t nvme_create_io_queues(struct nvme_device *device) {

}

static int32_t nvme_create_queue(struct nvme_device *device, int32_t queue_id) {

}

int32_t nvme_identify(struct nvme_device *nvme_dev, uint64_t namespace_id, uint64_t controller_or_namespace_identifier,
                      uint64_t dma_address);

int32_t nvme_init(struct device *dev) {
    struct nvme_device *nvme_dev = dev->device_info;

    nvme_dev->device = dev;
    doubly_linked_list_init(&nvme_dev->namespaces);

    nvme_dev->queues = kzmalloc(NVME_Q_NUM * sizeof(struct nvme_queue *));

    nvme_dev->capabilities = nvme_read_q(&nvme_dev->bar->capabilities);
    nvme_dev->doorbell_stride = 1 << NVME_CAP_STRIDE(nvme_dev->capabilities);
    nvme_dev->doorbells = (volatile uint32_t *) (nvme_dev->bar + 4096);


}