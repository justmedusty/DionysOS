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

// Enum representing NVMe opcodes used in commands
enum nvme_opcode {
    nvme_cmd_flush = 0x00,          // Flush command
    nvme_cmd_write = 0x01,          // Write command
    nvme_cmd_read = 0x02,           // Read command
    nvme_cmd_write_uncor = 0x04,    // Write Uncorrectable command
    nvme_cmd_compare = 0x05,        // Compare command
    nvme_cmd_write_zeroes = 0x08,   // Write Zeros command
    nvme_cmd_dsm = 0x09,            // Dataset Management command
    nvme_cmd_resv_register = 0x0d,  // Reservation Register command
    nvme_cmd_resv_report = 0x0e,    // Reservation Report command
    nvme_cmd_resv_acquire = 0x11,   // Reservation Acquire command
    nvme_cmd_resv_release = 0x15,   // Reservation Release command
};

// Struct representing a common NVMe command
struct nvme_common_command {
    uint8_t opcode;        // Command opcode
    uint8_t flags;         // Command flags
    uint16_t command_id;   // Command identifier
    uint32_t nsid;         // Namespace identifier
    uint32_t cdw2[2];      // Command specific double words
    uint64_t metadata;     // Metadata pointer
    uint64_t prp1;         // First PRP entry
    uint64_t prp2;         // Second PRP entry
    uint32_t cdw10[6];     // Command specific double words
};

// Struct representing a read/write NVMe command
struct nvme_rw_command {
    uint8_t opcode;        // Command opcode
    uint8_t flags;         // Command flags
    uint16_t command_id;   // Command identifier
    uint32_t nsid;         // Namespace identifier
    uint64_t rsvd2;        // Reserved
    uint64_t metadata;     // Metadata pointer
    uint64_t prp1;         // First PRP entry
    uint64_t prp2;         // Second PRP entry
    uint64_t slba;         // Starting LBA
    uint16_t length;       // Length of data transfer
    uint16_t control;      // Control flags
    uint32_t dsmgmt;       // Dataset management field
    uint32_t reftag;       // Reference tag
    uint16_t apptag;       // Application tag
    uint16_t appmask;      // Application tag mask
};

// Struct representing an NVMe identify command
struct nvme_identify {
    uint8_t opcode;        // Command opcode
    uint8_t flags;         // Command flags
    uint16_t command_id;   // Command identifier
    uint32_t nsid;         // Namespace identifier
    uint64_t rsvd2[2];     // Reserved
    uint64_t prp1;         // First PRP entry
    uint64_t prp2;         // Second PRP entry
    uint32_t cns;          // Controller or namespace structure identifier
    uint32_t rsvd11[5];    // Reserved
};

// Struct representing an NVMe features command
struct nvme_features {
    uint8_t opcode;        // Command opcode
    uint8_t flags;         // Command flags
    uint16_t command_id;   // Command identifier
    uint32_t nsid;         // Namespace identifier
    uint64_t rsvd2[2];     // Reserved
    uint64_t prp1;         // First PRP entry
    uint64_t prp2;         // Second PRP entry
    uint32_t fid;          // Feature identifier
    uint32_t dword11;      // Command specific double word
    uint32_t rsvd12[4];    // Reserved
};

// Struct representing an NVMe create completion queue command
struct nvme_create_cq {
    uint8_t opcode;        // Command opcode
    uint8_t flags;         // Command flags
    uint16_t command_id;   // Command identifier
    uint32_t rsvd1[5];     // Reserved
    uint64_t prp1;         // Physical Region Page entry 1
    uint64_t rsvd8;        // Reserved
    uint16_t cqid;         // Completion queue identifier
    uint16_t qsize;        // Queue size
    uint16_t cq_flags;     // Completion queue flags
    uint16_t irq_vector;   // Interrupt vector
    uint32_t rsvd12[4];    // Reserved
};

// Struct representing an NVMe create submission queue command
struct nvme_create_sq {
    uint8_t opcode;        // Command opcode
    uint8_t flags;         // Command flags
    uint16_t command_id;   // Command identifier
    uint32_t rsvd1[5];     // Reserved
    uint64_t prp1;         // Physical Region Page entry 1
    uint64_t rsvd8;        // Reserved
    uint16_t sqid;         // Submission queue identifier
    uint16_t qsize;        // Queue size
    uint16_t sq_flags;     // Submission queue flags
    uint16_t cqid;         // Completion queue identifier
    uint32_t rsvd12[4];    // Reserved
};

// Struct representing an NVMe delete queue command
struct nvme_delete_queue {
    uint8_t opcode;        // Command opcode
    uint8_t flags;         // Command flags
    uint16_t command_id;   // Command identifier
    uint32_t rsvd1[9];     // Reserved
    uint16_t qid;          // Queue identifier
    uint16_t rsvd10;       // Reserved
    uint32_t rsvd11[5];    // Reserved
};

// Struct representing an NVMe abort command
struct nvme_abort_cmd {
    uint8_t opcode;        // Command opcode
    uint8_t flags;         // Command flags
    uint16_t command_id;   // Command identifier
    uint32_t rsvd1[9];     // Reserved
    uint16_t sqid;         // Submission queue identifier
    uint16_t cid;          // Command identifier to abort
    uint32_t rsvd11[5];    // Reserved
};

// Struct representing an NVMe download firmware command
struct nvme_download_firmware {
    uint8_t opcode;        // Command opcode
    uint8_t flags;         // Command flags
    uint16_t command_id;   // Command identifier
    uint32_t rsvd1[5];     // Reserved
    uint64_t prp1;         // Physical Region Page entry 1
    uint64_t prp2;         // Physical Region Page entry 2
    uint32_t numd;         // Number of Dwords to download
    uint32_t offset;       // Offset within the firmware slot
    uint32_t rsvd12[4];    // Reserved
};

// Struct representing an NVMe format command
struct nvme_format_cmd {
    uint8_t opcode;        // Command opcode
    uint8_t flags;         // Command flags
    uint16_t command_id;   // Command identifier
    uint32_t nsid;         // Namespace identifier
    uint64_t rsvd2[4];     // Reserved
    uint32_t cdw10;        // Format command options
    uint32_t rsvd11[5];    // Reserved
};

// Union representing an NVMe command
struct nvme_command {
    union {
        struct nvme_common_command common;    // Common command format
        struct nvme_rw_command rw;            // Read/Write command format
        struct nvme_identify identify;        // Identify command format
        struct nvme_features features;        // Features command format
        struct nvme_create_cq create_cq;      // Create completion queue command
        struct nvme_create_sq create_sq;      // Create submission queue command
        struct nvme_delete_queue delete_queue; // Delete queue command
        struct nvme_download_firmware dlfw;   // Download firmware command
        struct nvme_format_cmd format;        // Format command
        struct nvme_abort_cmd abort;          // Abort command
    };
};

struct nvme_bar {
    uint64_t capabilities;       // Controller Capabilities
    uint32_t version;            // Version
    uint32_t interrupt_mask_set; // Interrupt Mask Set
    uint32_t interrupt_mask_clear; // Interrupt Mask Clear
    uint32_t controller_config;  // Controller Configuration
    uint32_t reserved1;          // Reserved
    uint32_t controller_status;  // Controller Status
    uint32_t reserved2;          // Reserved
    uint32_t admin_queue_attrs;  // Admin Queue Attributes
    uint64_t admin_sq_base_addr; // Admin Submission Queue Base Address
    uint64_t admin_cq_base_addr; // Admin Completion Queue Base Address
};


// Struct representing an NVMe completion queue entry
struct nvme_completion {
    uint32_t command_specific;  // Command specific information
    uint32_t reserved;          // Reserved
    uint16_t sq_head;           // Submission queue head pointer
    uint16_t sq_id;             // Submission queue identifier
    uint16_t command_id;        // Command identifier
    uint16_t status;            // Status code
};

// Enum for identifying queue types
enum nvme_queue_id {
    NVME_ADMIN_Q,  // Admin queue
    NVME_IO_Q,     // I/O queue
    NVME_Q_NUM,    // Total number of queues
};

// Struct representing an NVMe queue
struct nvme_queue {
    struct nvme_dev *dev;              // Associated NVMe device
    struct nvme_command *sq_cmds;      // Submission queue commands
    struct nvme_completion *cqes;      // Completion queue entries
    uint32_t *q_db;                    // Doorbell register
    uint16_t q_depth;                  // Queue depth
    int16_t cq_vector;                 // Completion queue interrupt vector
    uint16_t sq_head;                  // Submission queue head pointer
    uint16_t sq_tail;                  // Submission queue tail pointer
    uint16_t cq_head;                  // Completion queue head pointer
    uint16_t qid;                      // Queue identifier
    uint8_t cq_phase;                  // Completion queue phase bit
    uint8_t cqe_seen;                  // Indicates if a CQE was seen
    unsigned long cmdid_data[];        // Command ID data array
};

/* Represents an NVM Express device. Each nvme_dev is a PCI function. */
struct nvme_device {
    struct device *device;             // Pointer to the associated U-Boot device
    struct doubly_linked_list *node;        // Linked list node for device list
    struct nvme_queue **queue_list;      // Pointer to an array of NVMe queue pointers
    volatile uint32_t *doorbells;        // Pointer to doorbell registers
    int device_instance;                 // Instance identifier of the device
    unsigned int total_queues;           // Total number of queues
    unsigned int active_queues;          // Number of online queues
    unsigned int max_queue_id;           // Maximum queue identifier
    int queue_depth;                     // Depth of each queue
    uint32_t doorbell_stride;            // Stride between doorbell registers
    uint32_t controller_config;          // Controller configuration
    struct nvme_bar *bar;                // Pointer to Base Address Register (BAR) structure
    struct doubly_linked_list *namespace_list;     // Linked list of namespaces
    char vendor_id[8];                   // Vendor identifier
    char serial_number[20];              // Serial number
    char model_number[40];               // Model number
    char firmware_revision[8];           // Firmware revision
    uint32_t max_transfer_shift;         // Maximum transfer size shift
    uint64_t capabilities;               // Controller capabilities
    uint32_t stripe_size;                // Size of stripes for striping
    uint32_t page_size;                  // Page size
    uint8_t volatile_write_cache;        // Volatile write cache support
    uint64_t *prp_pool;                  // PRP (Physical Region Page) pool
    uint32_t prp_entry_count;            // Number of entries in the PRP pool
    uint32_t namespace_count;            // Number of namespaces
};

// Struct containing NVMe operations
struct nvme_ops {
    int32_t (*setup_queue)(struct nvme_queue *nvme_queue); // Setup queue function pointer
    void (*submit_cmd)(struct nvme_queue *nvme_queue, struct nvme_command *cmd); // Submit command function pointer
    void (*complete_cmd)(struct nvme_queue *nvme_queue, struct nvme_command *cmd); // Complete command function pointer
};

// Function prototypes
int32_t nvme_init(struct device *dev);        // Initialize NVMe device
int32_t nvme_shutdown(struct device *dev);    // Shutdown NVMe device

// Helper macros for NVMe queue and data size
#define NVME_ADMIN_QUEUE_SIZE 64                // Admin queue size
#define NVME_IO_QUEUE_SIZE 128                  // I/O queue size
#define NVME_PAGE_SHIFT 12                      // Page shift value (log2 of page size)
#define NVME_PAGE_SIZE (1 << NVME_PAGE_SHIFT)   // Page size
#define NVME_MAX_DATA_SIZE (NVME_PAGE_SIZE * NVME_IO_QUEUE_SIZE) // Maximum data size per I/O queue
#define NVME_LB_SIZE 512                        // Logical Block Size (common value)

#endif // NVME_H_
