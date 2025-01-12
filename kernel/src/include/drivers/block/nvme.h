//
// Created by dustyn on 9/17/24.
//

#ifndef NVME_H_
#define NVME_H_

#include <stdint.h>

// NVMe command opcodes
enum nvme_command_set {
    NVME_FLUSH = 0x00,
    NVME_WRITE = 0x01,
    NVME_READ = 0x02,
    NVME_WRITE_UNCORRECTABLE = 0x04,
    NVME_COMPARE = 0x05,
    NVME_WRITE_ZEROS = 0x08,
    NVME_DATASET_MANAGEMENT = 0x09,
    NVME_VERIFY = 0x0C,
    NVME_RESERVATION_REGISTER = 0x0D,
    NVME_RESERVATION_REPORT = 0x0E,
    NVME_RESERVATION_ACQUIRE = 0x11,
    NVME_RESERVATION_RELEASE = 0x15,
    NVME_CANCEL = 0x18,
    NVME_COPY = 0x19,
    NVME_GET_LOG_PAGE = 0x06,
    NVME_GET_LOG_PAGE_SIZE = 0x02,
    NVME_IDENTIFY = 0x06,
    NVME_ASYNC_EVENT_CONFIGURATION = 0x01,
    NVME_GET_LOG_PAGE_SIZE_EXT = 0x03,
};

// NVMe Status Codes
enum nvme_status_codes {
    NVME_SC_SUCCESS = 0x00,                  // Command completed successfully
    NVME_SC_INVALID_OPCODE = 0x01,           // Invalid opcode
    NVME_SC_INVALID_FIELD = 0x02,            // Invalid field in command
    NVME_SC_COMMAND_ID_CONFLICT = 0x03,      // Command ID conflict
    NVME_SC_DATA_TRANSFER_ERROR = 0x04,      // Data transfer error
    NVME_SC_ABORTED_POWER_LOSS = 0x05,       // Command aborted due to power loss
    NVME_SC_INTERNAL_DEVICE_ERROR = 0x06,    // Internal device error
    NVME_SC_ABORTED_BY_REQUEST = 0x07,       // Command aborted by request
    NVME_SC_ABORTED_SQ_DELETION = 0x08,      // Command aborted due to submission queue deletion
    NVME_SC_ABORTED_FAILED_FUSED = 0x09,     // Command aborted due to failed fused operation
    NVME_SC_ABORTED_MISSING_FUSED = 0x0A,    // Command aborted due to missing fused operation
    NVME_SC_INVALID_NAMESPACE_OR_FORMAT = 0x0B, // Invalid namespace or format
    NVME_SC_COMMAND_SEQUENCE_ERROR = 0x0C,   // Command sequence error
    NVME_SC_INVALID_SGL_SEGMENT_DESCRIPTOR = 0x0D, // Invalid SGL segment descriptor
    NVME_SC_INVALID_SGL_DESCRIPTOR_TYPE = 0x0E, // Invalid SGL descriptor type
    NVME_SC_INVALID_USE_OF_CMB = 0x0F,       // Invalid use of controller memory buffer
    NVME_SC_PRP_OFFSET_INVALID = 0x10,       // PRP offset invalid
    NVME_SC_ATOMIC_WRITE_UNIT_EXCEEDED = 0x14, // Atomic write unit exceeded
    NVME_SC_OPERATION_DENIED = 0x12,         // Operation denied
    NVME_SC_SGL_OFFSET_INVALID = 0x13,       // SGL offset invalid
    NVME_SC_INVALID_CONTROLLER_MEM_BUFFER = 0x14, // Invalid controller memory buffer
    NVME_SC_INVALID_COMMAND_SIZE = 0x15,     // Invalid command size
    NVME_SC_ABORTED_COMMAND_IN_PROGRESS = 0x16, // Command aborted while in progress
    NVME_SC_ABORTED_COMMAND_QUEUE_ERROR = 0x17, // Command aborted due to queue error
};

enum nvme_opcode {
    nvme_cmd_flush = 0x00,
    nvme_cmd_write = 0x01,
    nvme_cmd_read = 0x02,
    nvme_cmd_write_uncor = 0x04,
    nvme_cmd_compare = 0x05,
    nvme_cmd_write_zeroes = 0x08,
    nvme_cmd_dsm = 0x09,
    nvme_cmd_resv_register = 0x0d,
    nvme_cmd_resv_report = 0x0e,
    nvme_cmd_resv_acquire = 0x11,
    nvme_cmd_resv_release = 0x15,
};

struct nvme_common_command {
    uint8_t opcode;
    uint8_t flags;
    uint16_t command_id;
    uint32_t nsid;
    uint32_t cdw2[2];
    uint64_t metadata;
    uint64_t prp1;
    uint64_t prp2;
    uint32_t cdw10[6];
};

struct nvme_rw_command {
    uint8_t opcode;
    uint8_t flags;
    uint16_t command_id;
    uint32_t nsid;
    uint64_t rsvd2;
    uint64_t metadata;
    uint64_t prp1;
    uint64_t prp2;
    uint64_t slba;
    uint16_t length;
    uint16_t control;
    uint32_t dsmgmt;
    uint32_t reftag;
    uint16_t apptag;
    uint16_t appmask;
};
struct nvme_identify {
    uint8_t opcode;
    uint8_t flags;
    uint16_t command_id;
    uint32_t nsid;
    uint64_t rsvd2[2];
    uint64_t prp1;
    uint64_t prp2;
    uint32_t cns;
    uint32_t rsvd11[5];
};

struct nvme_features {
    uint8_t opcode;
    uint8_t flags;
    uint16_t command_id;
    uint32_t nsid;
    uint64_t rsvd2[2];
    uint64_t prp1;
    uint64_t prp2;
    uint32_t fid;
    uint32_t dword11;
    uint32_t rsvd12[4];
};

struct nvme_create_cq {
    uint8_t opcode;
    uint8_t flags;
    uint16_t command_id;
    uint32_t rsvd1[5];
    uint64_t prp1;
    uint64_t rsvd8;
    uint16_t cqid;
    uint16_t qsize;
    uint16_t cq_flags;
    uint16_t irq_vector;
    uint32_t rsvd12[4];
};

struct nvme_create_sq {
    uint8_t opcode;
    uint8_t flags;
    uint16_t command_id;
    uint32_t rsvd1[5];
    uint64_t prp1;
    uint64_t rsvd8;
    uint16_t sqid;
    uint16_t qsize;
    uint16_t sq_flags;
    uint16_t cqid;
    uint32_t rsvd12[4];
};

struct nvme_delete_queue {
    uint8_t opcode;
    uint8_t flags;
    uint16_t command_id;
    uint32_t rsvd1[9];
    uint16_t qid;
    uint16_t rsvd10;
    uint32_t rsvd11[5];
};

struct nvme_abort_cmd {
    uint8_t opcode;
    uint8_t flags;
    uint16_t command_id;
    uint32_t rsvd1[9];
    uint16_t sqid;
    uint16_t cid;
    uint32_t rsvd11[5];
};

struct nvme_download_firmware {
    uint8_t opcode;
    uint8_t flags;
    uint16_t command_id;
    uint32_t rsvd1[5];
    uint64_t prp1;
    uint64_t prp2;
    uint32_t numd;
    uint32_t offset;
    uint32_t rsvd12[4];
};

struct nvme_format_cmd {
    uint8_t opcode;
    uint8_t flags;
    uint16_t command_id;
    uint32_t nsid;
    uint64_t rsvd2[4];
    uint32_t cdw10;
    uint32_t rsvd11[5];
};

struct nvme_command {
    union {
        struct nvme_common_command common;
        struct nvme_rw_command rw;
        struct nvme_identify identify;
        struct nvme_features features;
        struct nvme_create_cq create_cq;
        struct nvme_create_sq create_sq;
        struct nvme_delete_queue delete_queue;
        struct nvme_download_firmware dlfw;
        struct nvme_format_cmd format;
        struct nvme_abort_cmd abort;
    };
};


// NVMe Completion Queue Entry
struct nvme_completion {
    uint32_t command_specific;
    uint32_t reserved;
    uint16_t sq_head;
    uint16_t sq_id;
    uint16_t command_id;
    uint16_t status;
};

/* Admin queue and a single I/O queue. */
enum nvme_queue_id {
    NVME_ADMIN_Q,
    NVME_IO_Q,
    NVME_Q_NUM,
};

/*
 * An NVM Express queue. Each device has at least two (one for admin
 * commands and one for I/O commands).
 */
struct nvme_queue {
    struct nvme_dev *dev;
    struct nvme_command *sq_cmds;
    struct nvme_completion *cqes;
    uint32_t  *q_db;
    uint16_t q_depth;
    int16_t cq_vector;
    uint16_t sq_head;
    uint16_t sq_tail;
    uint16_t cq_head;
    uint16_t qid;
    uint8_t cq_phase;
    uint8_t cqe_seen;
    unsigned long cmdid_data[];
};
struct nvme_device {
    struct device *device;

};


struct nvme_ops {

    int32_t (*setup_queue)(struct nvme_queue *nvme_queue);

    void (*submit_cmd)(struct nvme_queue *nvme_queue, struct nvme_command *cmd);

    void (*complete_cmd)(struct nvme_queue *nvme_queue, struct nvme_command *cmd);
};

int32_t nvme_init(struct device *dev);

int32_t nvme_shutdown(struct device *dev);


// Helper macros (adjust as needed)
#define NVME_ADMIN_QUEUE_SIZE 64
#define NVME_IO_QUEUE_SIZE 128
#define NVME_PAGE_SHIFT 12
#define NVME_PAGE_SIZE (1 << NVME_PAGE_SHIFT)
#define NVME_MAX_DATA_SIZE (NVME_PAGE_SIZE * NVME_IO_QUEUE_SIZE)
#define NVME_LB_SIZE 512 // Logical Block Size (common value)
#endif // NVME_H_
