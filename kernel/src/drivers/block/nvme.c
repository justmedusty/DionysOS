//
// Created by dustyn on 9/17/24.
//

#include <limits.h>
#include "include/drivers/block/nvme.h"
#include "include/device/device.h"
#include "include/memory/mem.h"
#include "include/architecture/arch_timer.h"


static int32_t
nvme_submit_sync_command(struct nvme_queue *queue, struct nvme_command *command, uint32_t *result, uint64_t timeout);

static uint64_t
nvme_write_block(struct device *dev, uint64_t block_number, uint64_t block_count, void *buffer);

static uint64_t
nvme_read_block(struct device *dev, uint64_t block_number, uint64_t block_count, void *buffer);

int32_t nvme_identify(struct nvme_device *nvme_dev, uint64_t namespace_id, uint64_t controller_or_namespace_identifier,
                      uint64_t dma_address);

static int32_t nvme_setup_io_queues(struct nvme_device *device);

static int32_t nvme_create_io_queues(struct nvme_device *device);

static int32_t nvme_delete_completion_queue(struct nvme_device *nvme_dev, uint16_t completion_queue_id);

static int32_t nvme_delete_submission_queue(struct nvme_device *nvme_dev, uint16_t submission_queue_id);

static int32_t nvme_create_queue(struct nvme_queue *queue, int32_t queue_id);

static int32_t nvme_alloc_submission_queue(struct nvme_device *dev, uint16_t queue_id,
                                           struct nvme_queue *queue);

static int32_t nvme_alloc_completion_queue(struct nvme_device *dev, uint16_t queue_id,
                                           struct nvme_queue *queue);

static int32_t nvme_delete_cq(struct nvme_device *dev, uint16_t completion_queue_id);

static int32_t nvme_delete_sq(struct nvme_device *dev, uint16_t submission_queue_id);

static int32_t nvme_delete_queue(struct nvme_device *nvme_dev, uint8_t opcode, uint16_t id);

static void nvme_init_queue(struct nvme_queue *queue, uint16_t queue_id);

static int32_t
nvme_submit_admin_command(struct nvme_device *nvme_device, struct nvme_command *command, uint32_t *result);

static void nvme_free_queues(struct nvme_device *nvme_dev, int32_t lowest);

static void nvme_free_queue(struct nvme_queue *queue);

int32_t nvme_get_features(struct nvme_device *device, uint64_t feature_id, uint64_t namespace_id, uint64_t dma_address,
                          uint32_t *result);

int32_t
nvme_set_features(struct nvme_device *nvme_device, uint64_t feature_id, uint64_t double_word11, uint64_t dma_address,
                  uint32_t *result);

static int32_t
nvme_setup_physical_region_pools(struct nvme_device *nvme_dev, uint64_t *physical_region_pool, int32_t total_length,
                                 uint64_t dma_address);

static int32_t nvme_set_queue_count(struct nvme_device *nvme_dev, int32_t count);


static uint16_t nvme_read_completion_status(struct nvme_queue *queue, uint16_t index);

static uint16_t nvme_get_command_id();

static void nvme_submit_command(struct nvme_queue *queue, struct nvme_command *command);

static int32_t nvme_configure_admin_queue(struct nvme_device *nvme_dev);

static int32_t nvme_disable_control(struct nvme_device *nvme_dev);

static int32_t nvme_enable_control(struct nvme_device *nvme_dev);

static struct nvme_queue *nvme_alloc_queue(struct nvme_device *nvme_dev, int32_t queue_id, int32_t depth);

static int32_t nvme_wait_ready(struct nvme_device *nvme_dev, bool enabled);

static uint64_t
nvme_raw_block(struct device *dev, uint64_t block_number, uint64_t block_count, void *buffer, bool write);

static int32_t nvme_setup_prps(struct nvme_device *nvme_dev, uint64_t *prp2,
                               int32_t total_len, uint64_t dma_addr);

/*
 * sets up prp entries for the given nvme device, handles allocation if needed.
 * fills out physical region pool 2 with the final address or null if not needed.
 */
static int32_t nvme_setup_prps(struct nvme_device *nvme_dev, uint64_t *prp2,
                               int32_t total_len, uint64_t dma_addr) {
    uint32_t page_size = nvme_dev->page_size;
    uint32_t offset = dma_addr & (page_size - 1);
    uint64_t *prp_pool;
    int32_t length = total_len;
    uint32_t i, number_prps;
    uint32_t prps_per_page = page_size >> 3;
    uint32_t num_pages;

    length -= (page_size - offset);

    if (length <= 0) {
        *prp2 = 0;
        return 0;
    }

    if (length)
        dma_addr += (page_size - offset);

    if (length <= page_size) {
        *prp2 = dma_addr;
        return 0;
    }

    number_prps = DIV_ROUND_UP(length, page_size);
    num_pages = DIV_ROUND_UP(number_prps, prps_per_page);

    if (number_prps > nvme_dev->prp_entry_count) {
        kfree(nvme_dev->prp_pool);
        /*
         * Always increase in increments of pages.  It doesn't waste
         * much memory and reduces the number of allocations.
         */
        nvme_dev->prp_pool = V2P(kzmalloc(num_pages * page_size));
        if (!nvme_dev->prp_pool) {
            err_printf("kmalloc prp_pool fail\n");
            return KERN_NO_MEM;
        }
        nvme_dev->prp_entry_count = prps_per_page * num_pages;
    }

    prp_pool = nvme_dev->prp_pool;
    i = 0;
    while (number_prps) {
        // if we've filled a page of physical region pool entries, link to the next page and reset index
        if (i == ((page_size >> 3) - 1)) {
            *(prp_pool + i) = (uint64_t) prp_pool + page_size; // point to next physical region pool page
            i = 0; // reset index for the new physical region page
            prp_pool += page_size; // move to the next physical region page
        }
        // set current prp entry to the dma address and increment index
        *(prp_pool + i++) = dma_addr;
        dma_addr += page_size; // move to the next dma address
        number_prps--; // decrement the number of prps left to set
    }

    *prp2 = (uint64_t) nvme_dev->prp_pool;


    return 0;
}


/*
 * reads or writes a raw block from/to the nvme device using the given buffer.
 * handles physical region pool setup and submits the command synchronously.
 */
static uint64_t
nvme_raw_block(struct device *dev, uint64_t block_number, uint64_t block_count, void *buffer, bool write) {
    struct nvme_namespace *ns = dev->device_info;
    struct nvme_device *nvme_dev = ns->device;
    struct nvme_command command;
    uint32_t status;

    uint64_t prp2;
    uint64_t total_len = block_count;
    uint64_t temp_len = total_len;
    uintptr_t temp_buffer = (uintptr_t) buffer;

    uint64_t slba = block_number;
    uint16_t lbas = 1 << (nvme_dev->max_transfer_shift - ns->logical_block_address_shift);
    uint64_t total_lbas = block_count;


    command.rw.opcode = write ? nvme_cmd_write : nvme_cmd_read;
    command.rw.flags = 0;
    command.rw.nsid = ns->namespace_id;
    command.rw.control = 0;
    command.rw.dsmgmt = 0;
    command.rw.reftag = 0;
    command.rw.apptag = 0;
    command.rw.appmask = 0;
    command.rw.metadata = 0;

    while (total_lbas) {
        if (total_lbas < lbas) {
            lbas = (uint16_t) total_lbas;
            total_lbas = 0;
        } else {
            total_lbas -= lbas;
        }

        if (nvme_setup_prps(nvme_dev, &prp2,
                            lbas << ns->logical_block_address_shift, temp_buffer)) {
            return KERN_IO_ERROR;
        }

        command.rw.slba = slba;
        slba += lbas;
        command.rw.length = lbas - 1;
        command.rw.prp1 = temp_buffer;
        command.rw.prp2 = prp2;
        status = nvme_submit_sync_command(nvme_dev->queues[NVME_IO_Q],
                                          &command, NULL, IO_TIMEOUT);
        if (status)
            break;
        temp_len -= (uint32_t) lbas << ns->logical_block_address_shift;
        temp_buffer += lbas << ns->logical_block_address_shift;
    }


    return (total_len - temp_len);
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

    nvme_dev->total_queues++;
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

/*
 * Frees an individual queue, free the completion queue entries, the submission queue commands, then finally the queue itself
 */
static void nvme_free_queue(struct nvme_queue *queue) {
    kfree(queue->cqes);
    kfree(queue->sq_cmds);
    kfree(queue);
}

/*
 * Iteratively free queues in the nvme device object
 */
static void nvme_free_queues(struct nvme_device *nvme_dev, int32_t lowest) {
    int32_t i;

    for (i = nvme_dev->total_queues; i >= lowest; i--) {
        struct nvme_queue *queue = nvme_dev->queues[i];
        nvme_dev->total_queues--;
        nvme_dev->queues[i] = NULL; // so we dont have a dangling pointer
        nvme_free_queue(queue);
    }
}


/*
 * Submit a command in the admin queue, the timeouts are meant to be us not ms but for now I don't care this is fine
 */
static int32_t
nvme_submit_admin_command(struct nvme_device *nvme_device, struct nvme_command *command, uint32_t *result) {
    return nvme_submit_sync_command(nvme_device->queues[NVME_ADMIN_Q], command, result, ADMIN_TIMEOUT);
}


static int32_t nvme_get_info_from_identify(struct nvme_device *device) {


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

/*
 * Send an admin command to delete a queue
 */
static int32_t nvme_delete_queue(struct nvme_device *nvme_dev, uint8_t opcode, uint16_t id) {

    struct nvme_command command;
    memset(&command, 0, sizeof(command));
    command.delete_queue.opcode = opcode;
    command.delete_queue.qid = id;

    return nvme_submit_admin_command(nvme_dev, &command, NULL);


}
/*
 * Delete a submission queue
 */
static int32_t nvme_delete_sq(struct nvme_device *dev, uint16_t submission_queue_id) {
    return nvme_delete_queue(dev, NVME_ADMIN_OPCODE_DELETE_SQ, submission_queue_id);
}
/*
 * Delete a command queue
 */
static int32_t nvme_delete_cq(struct nvme_device *dev, uint16_t completion_queue_id) {
    return nvme_delete_queue(dev, NVME_ADMIN_OPCODE_DELETE_CQ, completion_queue_id);
}

/*
 * allocates a new completion queue via submission of an admin command
 */
static int32_t nvme_alloc_completion_queue(struct nvme_device *dev, uint16_t queue_id,
                                           struct nvme_queue *queue) {
    struct nvme_command command;
    int32_t flags = NVME_QUEUE_PHYS_CONTIG | NVME_CQ_IRQ_ENABLED;

    memset(&command, 0, sizeof(command));
    command.create_cq.opcode = NVME_ADMIN_OPCODE_CREATE_CQ;
    command.create_cq.prp1 = ((uint64_t) queue->cqes);
    command.create_cq.cqid = (queue_id);
    command.create_cq.qsize = (queue->q_depth - 1);
    command.create_cq.cq_flags = (flags);
    command.create_cq.irq_vector = (queue->cq_vector);

    return nvme_submit_admin_command(dev, &command, NULL);
}

/*
 * allocates a new submission queue via submission of an admin command
 */
static int32_t nvme_alloc_submission_queue(struct nvme_device *dev, uint16_t queue_id,
                                           struct nvme_queue *queue) {
    struct nvme_command command;
    int32_t flags = NVME_QUEUE_PHYS_CONTIG | NVME_CQ_IRQ_ENABLED;

    memset(&command, 0, sizeof(command));
    command.create_sq.opcode = NVME_ADMIN_OPCODE_CREATE_SQ;
    command.create_sq.prp1 = ((uint64_t) queue->sq_cmds);
    command.create_sq.sqid = (queue_id);
    command.create_sq.qsize = (queue->q_depth - 1);
    command.create_sq.sq_flags = (flags);
    command.create_sq.cqid = (queue_id);

    return nvme_submit_admin_command(dev, &command, NULL);
}

static int32_t nvme_create_queue(struct nvme_queue *queue, int32_t queue_id) {

    struct nvme_device *dev = queue->dev;
    int result;

    queue->cq_vector = queue_id - 1;
    result = nvme_alloc_completion_queue(dev, queue_id, queue);
    if (result < 0)
        goto release_cq;

    result = nvme_alloc_submission_queue(dev, queue_id, queue);
    if (result < 0)
        goto release_sq;

    nvme_init_queue(queue, queue_id);

    return result;

    release_sq:
    nvme_delete_sq(dev, queue_id);
    release_cq:
    nvme_delete_cq(dev, queue_id);

    return result;

}

/*
 * Deletes a submission queue of the passed id on the passed nvme device
 */
static int32_t nvme_delete_submission_queue(struct nvme_device *nvme_dev, uint16_t submission_queue_id) {
    return nvme_delete_queue(nvme_dev, NVME_ADMIN_OPCODE_CREATE_SQ, submission_queue_id);
}

/*
 * Deletes a completion queue of the passed id on the passed nvme device
 */
static int32_t nvme_delete_completion_queue(struct nvme_device *nvme_dev, uint16_t completion_queue_id) {
    return nvme_delete_queue(nvme_dev, NVME_ADMIN_OPCODE_DELETE_CQ, completion_queue_id);
}

static int32_t nvme_create_io_queues(struct nvme_device *device) {

    uint32_t i;

    for (i = device->total_queues; i <= device->max_queue_id; i++) {
        if (!nvme_alloc_queue(device, i, device->queue_depth)) {
            break;
        }
    }

    for (i = device->active_queues; i <= device->total_queues; i++) {
        if (nvme_create_queue(device->queues[i], i)) {
            break;
        }
    }
}

static int32_t nvme_setup_io_queues(struct nvme_device *device) {

    int32_t number_io_queues;
    int32_t result;

    number_io_queues = 1;
    result = nvme_set_queue_count(device, number_io_queues);

    if (result <= 0) {
        return result;
    }
    //free previously allocated queues
    nvme_free_queues(device, number_io_queues + 1);
    nvme_create_io_queues(device);

    return 0;
}

int32_t nvme_identify(struct nvme_device *nvme_dev, uint64_t namespace_id, uint64_t controller_or_namespace_identifier,
                      uint64_t dma_address) {

}

/*
 * Read and write to/from buffers respectively
 */
static uint64_t
nvme_read_block(struct device *dev, uint64_t block_number, uint64_t block_count, void *buffer) {
    return nvme_raw_block(dev, block_number, block_count, buffer, false);
}

static uint64_t
nvme_write_block(struct device *dev, uint64_t block_number, uint64_t block_count, void *buffer) {
    return nvme_raw_block(dev, block_number, block_count, buffer, true);
}

int32_t nvme_init(struct device *dev) {
    struct nvme_device *nvme_dev = dev->device_info;

    nvme_dev->device = dev;
    doubly_linked_list_init(&nvme_dev->namespaces);

    nvme_dev->queues = kzmalloc(NVME_Q_NUM * sizeof(struct nvme_queue *));

    nvme_dev->capabilities = nvme_read_q(&nvme_dev->bar->capabilities);
    nvme_dev->doorbell_stride = 1 << NVME_CAP_STRIDE(nvme_dev->capabilities);
    nvme_dev->doorbells = (volatile uint32_t *) (nvme_dev->bar + 4096);


}

int32_t nvme_shutdown(struct device *dev) {
    struct nvme_device *nvme_dev = dev->device_info;

    return nvme_disable_control(nvme_dev);

}