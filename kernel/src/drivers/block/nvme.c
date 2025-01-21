//
// Created by dustyn on 9/17/24.
//

#include <limits.h>
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

/*
 * Wait for the NVMe device to either be enabled or disabled on a 500 millis timeout
 */
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

/*
 * Allocate a new nvme queue and attach it to the nvme device we have attached,
 *
 */
static struct nvme_queue *nvme_alloc_queue(struct nvme_device *nvme_dev, int32_t queue_id, int32_t depth) {
    struct nvme_ops *ops;
    struct nvme_queue *queue = kzmalloc(sizeof(struct nvme_queue));

    queue->cqes = kzmalloc(PAGE_SIZE);

    queue->sq_cmds = kzmalloc(PAGE_SIZE);

    queue->dev = nvme_dev;

    queue->cq_head = 0;
    queue->cq_phase = 1;
    queue->q_db = &nvme_dev->doorbells[queue_id * 2 * nvme_dev->doorbell_stride];

    queue->q_depth = depth;

    queue->qid = queue_id;

    nvme_dev->active_queues++;
    nvme_dev->queues[queue_id] = queue;

    ops = (struct nvme_ops *) nvme_dev->device->device_ops;

    if (ops && ops->setup_queue) {
        ops->setup_queue(queue);
    }

    return queue;
}

/*
 * Enable nvme controller
 * returns KERN_SUCCESS (0) on success, KERN_TIMOUT on failure
 */
static int32_t nvme_enable_control(struct nvme_device *nvme_dev) {

    nvme_dev->controller_config &= ~NVME_CC_SHN_MASK;
    nvme_dev->controller_config |= NVME_CC_ENABLE;

    nvme_dev->bar->controller_config = nvme_dev->controller_config;

    return nvme_wait_ready(nvme_dev, true);


}

/*
 * Disable nvme controller
 * returns KERN_SUCCESS (0) on success, KERN_TIMOUT on failure
 */
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
        return KERN_NO_DEVICE;
    }


    if (page_shift < device_page_max) {
        page_shift = device_page_max;
    }


    result = nvme_disable_control(nvme_dev);

    if (result < 0) {
        return result;
    }

    queue = nvme_dev->queues[NVME_ADMIN_Q];

    if (!queue) {

    }

}

static int32_t
nvme_submit_admin_command(struct nvme_device *nvme_device, struct nvme_command *command, uint32_t *result) {

}

/*
 * Submit a command to the controllers submission queue, handle wraparound if the queue goes beyond the depth
 */
static void nvme_submit_command(struct nvme_queue *queue, struct nvme_command *command) {

    struct nvme_ops *ops;

    uint64_t tail = queue->sq_tail;

    memcpy(&queue->sq_cmds[tail], command, sizeof(*command));


    ops = (struct nvme_ops *) queue->dev->device->device_ops->nvme_ops;

    if (ops && ops->submit_cmd) {
        ops->submit_cmd(queue, command);
        return;
    }

    if (++tail == queue->q_depth) {
        tail = 0;
    }
    *queue->q_db = tail;

    queue->sq_tail = tail;
}

static int32_t nvme_set_queue_count(struct nvme_device *nvme_dev, int32_t count) {

}

int32_t nvme_scan_namespace() {

}

static int32_t
nvme_setup_physical_region_pools(struct nvme_device *nvme_dev, uint64_t *physical_region_pool, int32_t total_length,
                                 uint64_t dma_address) {

}

/*
 * Read the status index in the completion queue to so we can see what is going on, hopefully no alignment issues but we will see won't we
 */
static uint16_t nvme_read_completion_status(struct nvme_queue *queue, uint16_t index) {

    uint64_t start = (uint64_t) &queue->cqes[0];

    uint64_t stop = start + NVME_CQ_SIZE(
            queue->q_depth); // this might cause alignment issues but we're fucking cowboys here okay?!

    return queue->cqes[index].status;


}

int32_t
nvme_set_features(struct nvme_device *nvme_device, uint64_t feature_id, uint64_t double_word11, uint64_t dma_address,
                  uint32_t *result) {

}

int32_t nvme_get_features(struct nvme_device *device, uint64_t feature_id, uint64_t namespace_id, uint64_t dma_address,
                          uint32_t *result) {

}

/*
 * Just get a command id and wraparound once we hit max
 */
static uint16_t nvme_get_command_id() {
    static uint16_t command_id;

    return command_id < USHRT_MAX ? command_id++ : 0;
}

/*
 * Submits a synchronous nvme command to the controller's queue.
 * Waits for the command to complete or times out after the specified duration.
 * Handles the wraparound of the completion queue (CQ) head and phase.
 */
static int32_t
nvme_submit_sync_command(struct nvme_queue *queue, struct nvme_command *command, uint32_t *result, uint64_t timeout) {

    struct nvme_ops *ops;
    uint16_t head = queue->cq_head; // Current head of the completion queue
    uint16_t phase = queue->cq_phase; // Current phase of the  completion queue

    uint16_t status; // Status of the command
    uint64_t start_time; // Start time for timeout tracking

    // Assign a unique command ID to the command and handle wraparound if necessary
    command->common.command_id = nvme_get_command_id();

    // Get the current timer count to track the timeout
    start_time = timer_get_current_count();

    for (;;) {
        // Read the status of the command completion
        status = nvme_read_completion_status(queue, head);

        // Check if the status phase matches the current phase
        if ((status & 1) == phase) {
            break;
        }

        // Check for timeout
        if (timeout > 0 && (timer_get_current_count() - start_time) >= timeout) {
            return KERN_TIMEOUT; // Return timeout error if elapsed time exceeds the specified timeout
        }
    }

    // Check for custom nvme operations to complete the command
    ops = queue->dev->device->device_ops->nvme_ops;
    if (ops && ops->complete_cmd) {
        ops->complete_cmd(queue, command); // Use the custom operation if available
    }

    // Shift the status to get the actual status code
    status >>= 1;
    if (status) {
        // Print error details if status indicates an error
        err_printf("status = %x, phase = %i, head = %i\n", status, phase, head);
        status = 0;

        // Update the CQ head and phase, wrapping around if necessary
        if (++head == queue->q_depth) {
            head = 0;
            phase = !phase;
        }

        // Update the  completion queue doorbell with the new head position
        *(queue->q_db + queue->dev->doorbell_stride) = head;
        queue->cq_head = head;
        queue->cq_phase = phase;

        return KERN_IO_ERROR; // Return IO error if status indicates failure
    }

    // If result pointer is provided, store the command result
    if (result) {
        *result = queue->cqes[head].result;
    }

    // Update the  completion queue  head and phase, wrapping around if necessary
    if (++head == queue->q_depth) {
        head = 0;
        phase = !phase;
    }

    // Update the  completion queue  doorbell with the new head position
    *(queue->q_db + queue->dev->doorbell_stride) = head;
    queue->cq_head = head;
    queue->cq_phase = phase;

    return status; // Return the status of the command
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