//
// Created by dustyn on 9/17/24.
//

#include "include/drivers/block/nvme.h"
#include "include/device/device.h"
#include "include/memory/mem.h"
#include "include/architecture/arch_timer.h"

static uint64_t nvme_read_raw_block(struct device *dev, uint64_t block_number, uint64_t block_count, void *buffer) {
    struct nvme_device *nvme_dev = dev->device_info;
    struct nvme_command command;
    uint32_t status;


}


static uint64_t nvme_write_raw_block(struct device *dev, uint64_t block_number, uint64_t block_count, void *buffer) {

}

static int32_t nvme_wait_ready(struct nvme_device *nvme_dev, bool enabled) {
    uint32_t bit = enabled ? NVME_CSTS_RDY : 0;
    int32_t timeout_millis;

    int64_t start;

    timeout_millis = NVME_CAP_TIMEOUT(nvme_dev->capabilities) * 500;

    start = timer_get_current_count();

    while (timer_get_current_count() - start < timeout_millis) {
        if ((nvme_dev->bar->controller_status & NVME_CSTS_RDY) == bit) {
            return KERN_SUCCESS;
        }
    }

    return KERN_TIMEOUT;

}

static int32_t nvme_enable_control(struct nvme_device *nvme_dev) {

    nvme_dev->controller_config &= ~NVME_CC_SHN_MASK;
    nvme_dev->controller_config |= NVME_CC_ENABLE;

    nvme_dev->bar->controller_config = nvme_dev->controller_config;

    return nvme_wait_ready(nvme_dev, true);


}

static int32_t nvme_disable_control(struct nvme_device *nvme_dev) {


    nvme_dev->controller_config &= ~NVME_CC_SHN_MASK;
    nvme_dev->controller_config &= ~NVME_CC_ENABLE;

    nvme_dev->bar->controller_config = nvme_dev->controller_config;

    return nvme_wait_ready(nvme_dev, false);

}

static int32_t nvme_configure_admin_queue(struct nvme_device *nvme_dev) {

    int32_t result;
    uint32_t aqa;
    uint64_t capabilities = nvme_dev->capabilities;

    struct nvme_queue *queue;

    uint64_t page_shift = 12;
    uint64_t device_page_min = NVME_CAP_MPSMIN(capabilities) + 12;
    uint64_t device_page_max = NVME_CAP_MPSMAX(capabilities) + 12;

    if (page_shift < device_page_min) {

    }


    if (page_shift < device_page_max) {

    }

}

static int32_t
nvme_submit_admin_command(struct nvme_device *nvme_device, struct nvme_command *command, uint32_t *result) {

}


static void nvme_submit_command(struct nvme_queue *queue, struct nvme_command *command) {

}

static int32_t nvme_set_queue_count(struct nvme_device *nvme_dev, int32_t count) {

}

int32_t nvme_scan_namespace() {

}

static int32_t
nvme_setup_physical_region_pools(struct nvme_device *nvme_dev, uint64_t *physical_region_pool, int32_t total_length,
                                 uint64_t dma_address) {

}

static uint16_t nvme_read_completion_status(struct nvme_queue *queue, uint16_t index) {

}

int32_t
nvme_set_features(struct nvme_device *nvme_device, uint64_t feature_id, uint64_t double_word11, uint64_t dma_address,
                  uint32_t *result) {

}

int32_t nvme_get_features(struct nvme_device *device, uint64_t feature_id, uint64_t namespace_id, uint64_t dma_address,
                          uint32_t *result) {

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

/*
 * Set up an nvme queue, set the submission queue head and tail to 0, set the completion queue phase bit to 1, set the doorbell register, and clear the completion queue entries
 */
static void nvme_init_queue(struct nvme_queue *queue, uint16_t queue_id) {

    struct nvme_device *nvme_dev = queue->dev;

    queue->sq_head = 0; //set submission queue header pointer to 0
    queue->sq_tail = 0; // set submission queue tail pointer to zero
    queue->cq_phase = 1;
    queue->q_db = &nvme_dev->doorbells[queue_id * 2 *
                                       nvme_dev->doorbell_stride]; // set the doorbell for the queue based on the queue_id in the list of doorbells on the device
    memset(queue->cqes, 0, NVME_CQ_SIZE(queue->q_depth));

    nvme_dev->active_queues++;
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