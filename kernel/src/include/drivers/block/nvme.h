//
// Created by dustyn on 9/17/24.
//

#ifndef NVME_H_
#define NVME_H_

#include <stdint.h>
#include "include/data_structures/doubly_linked_list.h"
#include "include/drivers/bus/pci.h"

#define NVME_QUEUE_DEPTH 2
#define ADMIN_TIMEOUT 60
#define IO_TIMEOUT 30
#define MAX_PRP_POOL 512
#define NVME_CQ_SIZE(depth) (depth * sizeof(struct nvme_command))
#define NVME_SQ_SIZE(depth) (depth * sizeof(struct nvme_command))

#define NVME_PCI_CLASS 1
#define NVME_PCI_SUBCLASS 8
#define IS_NVME_CONTROLLER(pci_device) (pci_device->class == NVME_PCI_CLASS && pci_device->subclass == NVME_PCI_SUBCLASS)
enum {
    // Generic Command Statuses
    NVME_SC_CMDID_CONFLICT = 0x3,   // Command identifier conflict
    NVME_SC_DATA_XFER_ERROR = 0x4,   // Data transfer error
    NVME_SC_POWER_LOSS = 0x5,   // Power loss or reset occurred
    NVME_SC_INTERNAL = 0x6,   // Internal error
    NVME_SC_ABORT_REQ = 0x7,   // Command aborted by request
    NVME_SC_ABORT_QUEUE = 0x8,   // Command aborted due to queue deletion
    NVME_SC_FUSED_FAIL = 0x9,   // Fused command failure
    NVME_SC_FUSED_MISSING = 0xa,   // Fused command missing
    NVME_SC_INVALID_NS = 0xb,   // Invalid namespace or format
    NVME_SC_CMD_SEQ_ERROR = 0xc,   // Command sequence error
    NVME_SC_SGL_INVALID_LAST = 0xd,   // Invalid last SGL segment descriptor
    NVME_SC_SGL_INVALID_COUNT = 0xe,   // Invalid SGL segment descriptor count
    NVME_SC_SGL_INVALID_DATA = 0xf,   // Invalid data in SGL
    NVME_SC_SGL_INVALID_METADATA = 0x10,  // Invalid metadata in SGL
    NVME_SC_SGL_INVALID_TYPE = 0x11,  // Invalid SGL segment descriptor type

    // Command Specific Statuses
    NVME_SC_LBA_RANGE = 0x80,  // LBA range error
    NVME_SC_CAP_EXCEEDED = 0x81,  // Capacity exceeded
    NVME_SC_NS_NOT_READY = 0x82,  // Namespace not ready
    NVME_SC_RESERVATION_CONFLICT = 0x83,  // Reservation conflict

    // Queue Management Statuses
    NVME_SC_CQ_INVALID = 0x100, // Completion queue invalid
    NVME_SC_QID_INVALID = 0x101, // Invalid queue identifier
    NVME_SC_QUEUE_SIZE = 0x102, // Queue size exceeds limit
    NVME_SC_ABORT_LIMIT = 0x103, // Abort command limit exceeded
    NVME_SC_ABORT_MISSING = 0x104, // Missing abort command
    NVME_SC_ASYNC_LIMIT = 0x105, // Asynchronous event request limit exceeded
    NVME_SC_FIRMWARE_SLOT = 0x106, // Invalid firmware slot
    NVME_SC_FIRMWARE_IMAGE = 0x107, // Invalid firmware image
    NVME_SC_INVALID_VECTOR = 0x108, // Invalid interrupt vector
    NVME_SC_INVALID_LOG_PAGE = 0x109, // Invalid log page
    NVME_SC_INVALID_FORMAT = 0x10a, // Invalid format
    NVME_SC_FIRMWARE_NEEDS_RESET = 0x10b, // Firmware activation requires reset
    NVME_SC_INVALID_QUEUE = 0x10c, // Invalid queue
    NVME_SC_FEATURE_NOT_SAVEABLE = 0x10d, // Feature identifier not saveable
    NVME_SC_FEATURE_NOT_CHANGEABLE = 0x10e, // Feature identifier not changeable
    NVME_SC_FEATURE_NOT_PER_NS = 0x10f, // Feature identifier not namespace-specific
    NVME_SC_FW_NEEDS_RESET_SUBSYS = 0x110, // Firmware activation requires subsystem reset

    // Metadata and Protection Information Statuses
    NVME_SC_BAD_ATTRIBUTES = 0x180, // Bad attributes
    NVME_SC_INVALID_PI = 0x181, // Invalid Protection Information (PI)
    NVME_SC_READ_ONLY = 0x182, // Write operation attempted on read-only resource

    // Media Errors
    NVME_SC_WRITE_FAULT = 0x280, // Write fault
    NVME_SC_READ_ERROR = 0x281, // Unrecovered read error
    NVME_SC_GUARD_CHECK = 0x282, // Guard check error
    NVME_SC_APPTAG_CHECK = 0x283, // Application tag check error
    NVME_SC_REFTAG_CHECK = 0x284, // Reference tag check error
    NVME_SC_COMPARE_FAILED = 0x285, // Compare operation failed
    NVME_SC_ACCESS_DENIED = 0x286, // Access denied

    // Do Not Retry Status
    NVME_SC_DNR = 0x4000 // Do Not Retry: Fatal error, do not retry command
};

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

enum nvme_admin_opcode {
    NVME_ADMIN_OPCODE_DELETE_SQ = 0x00, // Delete Submission Queue
    NVME_ADMIN_OPCODE_CREATE_SQ = 0x01, // Create Submission Queue
    NVME_ADMIN_OPCODE_GET_LOG_PAGE = 0x02, // Get Log Page
    NVME_ADMIN_OPCODE_DELETE_CQ = 0x04, // Delete Completion Queue
    NVME_ADMIN_OPCODE_CREATE_CQ = 0x05, // Create Completion Queue
    NVME_ADMIN_OPCODE_IDENTIFY = 0x06, // Identify
    NVME_ADMIN_OPCODE_ABORT_CMD = 0x08, // Abort Command
    NVME_ADMIN_OPCODE_SET_FEATURES = 0x09, // Set Features
    NVME_ADMIN_OPCODE_GET_FEATURES = 0x0A, // Get Features
    NVME_ADMIN_OPCODE_ASYNC_EVENT = 0x0C, // Asynchronous Event
    NVME_ADMIN_OPCODE_ACTIVATE_FW = 0x10, // Activate Firmware
    NVME_ADMIN_OPCODE_DOWNLOAD_FW = 0x11, // Download Firmware
    NVME_ADMIN_OPCODE_FORMAT_NVM = 0x80, // Format NVM
    NVME_ADMIN_OPCODE_SECURITY_SEND = 0x81, // Security Send
    NVME_ADMIN_OPCODE_SECURITY_RECEIVE = 0x82, // Security Receive
};

enum {
    NVME_QUEUE_PHYS_CONTIG = (1 << 0),
    NVME_CQ_IRQ_ENABLED = (1 << 1),
    NVME_SQ_PRIO_URGENT = (0 << 1),
    NVME_SQ_PRIO_HIGH = (1 << 1),
    NVME_SQ_PRIO_MEDIUM = (2 << 1),
    NVME_SQ_PRIO_LOW = (3 << 1),
    NVME_FEAT_ARBITRATION = 0x01,
    NVME_FEAT_POWER_MGMT = 0x02,
    NVME_FEAT_LBA_RANGE = 0x03,
    NVME_FEAT_TEMP_THRESH = 0x04,
    NVME_FEAT_ERR_RECOVERY = 0x05,
    NVME_FEAT_VOLATILE_WC = 0x06,
    NVME_FEAT_NUM_QUEUES = 0x07,
    NVME_FEAT_IRQ_COALESCE = 0x08,
    NVME_FEAT_IRQ_CONFIG = 0x09,
    NVME_FEAT_WRITE_ATOMIC = 0x0a,
    NVME_FEAT_ASYNC_EVENT = 0x0b,
    NVME_FEAT_AUTO_PST = 0x0c,
    NVME_FEAT_SW_PROGRESS = 0x80,
    NVME_FEAT_HOST_ID = 0x81,
    NVME_FEAT_RESV_MASK = 0x82,
    NVME_FEAT_RESV_PERSIST = 0x83,
    NVME_LOG_ERROR = 0x01,
    NVME_LOG_SMART = 0x02,
    NVME_LOG_FW_SLOT = 0x03,
    NVME_LOG_RESERVATION = 0x80,
    NVME_FWACT_REPL = (0 << 3),
    NVME_FWACT_REPL_ACTV = (1 << 3),
    NVME_FWACT_ACTV = (2 << 3),
};
enum {
    NVME_NS_FEATURE_THIN_PROVISIONING = 1 << 0, // Namespace supports thin provisioning
    NVME_BS_FLBAS_LBA_MASK = 0xf,    // LBA format mask
    NVME_NS_FLBAS_METADATA_EXTENDED = 0x10,   // Metadata extended LBA
    NVME_LBAF_RELIABILITY_BEST = 0,      // Best reliability level
    NVME_LBAF_RELIABILITY_BETTER = 1,      // Better reliability level
    NVME_LBAF_RELIABILITY_GOOD = 2,      // Good reliability level
    NVME_LBAF_RELIABILITY_DEGRADED = 3,      // Degraded reliability level
    NVME_NS_DPC_PROTECTION_INFO_LAST = 1 << 4, // Data Protection Capability: PI last
    NVME_NS_DPC_PROTECTION_INFO_FIRST = 1 << 3, // Data Protection Capability: PI first
    NVME_NS_DPC_PROTECTION_INFO_TYPE3 = 1 << 2, // Data Protection Capability: PI type 3
    NVME_NS_DPC_PROTECTION_INFO_TYPE2 = 1 << 1, // Data Protection Capability: PI type 2
    NVME_NS_DPC_PROTECTION_INFO_TYPE1 = 1 << 0, // Data Protection Capability: PI type 1
    NVME_NS_DPS_PROTECTION_INFO_FIRST = 1 << 3, // Data Protection Settings: PI first
    NVME_NS_DPS_PROTECTION_INFO_MASK = 0x7,    // Data Protection Settings: PI mask
    NVME_NS_DPS_PROTECTION_INFO_TYPE1 = 1,      // Data Protection Settings: PI type 1
    NVME_NS_DPS_PROTECTION_INFO_TYPE2 = 2,      // Data Protection Settings: PI type 2
    NVME_NS_DPS_PROTECTION_INFO_TYPE3 = 3,      // Data Protection Settings: PI type 3
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

enum {
    // Controller Configuration (CC) Register Flags
    NVME_CC_ENABLE = 1 << 0,  // Enable controller
    NVME_CC_CSS_NVM = 0 << 4,  // Command Set Selected: NVM Command Set
    NVME_CC_MPS_SHIFT = 7,       // Memory Page Size Shift
    NVME_CC_ARB_RR = 0 << 11, // Arbitration Mechanism: Round Robin
    NVME_CC_ARB_WRRU = 1 << 11, // Arbitration Mechanism: Weighted Round Robin with Urgent
    NVME_CC_ARB_VS = 7 << 11, // Arbitration Mechanism: Vendor Specific

    // Shutdown Notification (SHN) Flags
    NVME_CC_SHN_NONE = 0 << 14, // No shutdown notification
    NVME_CC_SHN_NORMAL = 1 << 14, // Normal shutdown notification
    NVME_CC_SHN_ABRUPT = 2 << 14, // Abrupt shutdown notification
    NVME_CC_SHN_MASK = 3 << 14, // Mask for shutdown notification

    // I/O Submission and Completion Queue Entry Sizes
    NVME_CC_IOSQES = 6 << 16, // I/O Submission Queue Entry Size
    NVME_CC_IOCQES = 4 << 20, // I/O Completion Queue Entry Size

    // Controller Status (CSTS) Register Flags
    NVME_CSTS_RDY = 1 << 0,  // Controller Ready
    NVME_CSTS_CFS = 1 << 1,  // Controller Fatal Status
    NVME_CSTS_SHST_NORMAL = 0 << 2,  // Shutdown Status: Normal operation
    NVME_CSTS_SHST_OCCUR = 1 << 2,  // Shutdown Status: Shutdown in progress
    NVME_CSTS_SHST_CMPLT = 2 << 2,  // Shutdown Status: Shutdown completed
    NVME_CSTS_SHST_MASK = 3 << 2,  // Mask for shutdown status
};

#include <stdint.h>

struct nvme_id_power_state {
    uint16_t max_power_cw;          /* Max power in centiwatts */
    uint8_t reserved2;
    uint8_t flags;
    uint32_t entry_latency_us;      /* Entry latency in microseconds */
    uint32_t exit_latency_us;       /* Exit latency in microseconds */
    uint8_t read_throughput;
    uint8_t read_latency;
    uint8_t write_throughput;
    uint8_t write_latency;
    uint16_t idle_power;
    uint8_t idle_power_scale;
    uint8_t reserved19;
    uint16_t active_power;
    uint8_t active_work_scale;
    uint8_t reserved23[9];
};

// NVMe Power State Flags
enum {
    NVME_PS_FLAGS_MAX_POWER_SCALE = 1 << 0, // Indicates the max power is scaled
    NVME_PS_FLAGS_NON_OP_STATE = 1 << 1,    // Non-operational state
};

// NVMe Controller Identification Structure
struct nvme_id_ctrl {
    uint16_t vendor_id;                  // Vendor ID
    uint16_t subsystem_vendor_id;        // Subsystem Vendor ID
    char serial_number[20];              // Serial Number
    char model_number[40];               // Model Number
    char firmware_revision[8];           // Firmware Revision
    uint8_t recommended_arbitration_burst; // Recommended Arbitration Burst
    uint8_t ieee_oui[3];                 // IEEE Organizationally Unique Identifier
    uint8_t max_data_transfer_size;      // Max Data Transfer Size
    uint8_t max_directive_transfer_size; // Max Directive Transfer Size
    uint16_t controller_id;              // Controller ID
    uint32_t version;                    // NVM Express version
    uint8_t reserved84[172];             // Reserved
    uint16_t optional_admin_command_support; // Optional Admin Command Support
    uint8_t abort_command_limit;         // Abort Command Limit
    uint8_t async_event_request_limit;   // Asynchronous Event Request Limit
    uint8_t firmware_updates;            // Firmware Updates Support
    uint8_t log_page_attributes;         // Log Page Attributes
    uint8_t error_log_page_entries;      // Error Log Page Entries
    uint8_t num_power_states_supported;  // Number of Power States Supported
    uint8_t admin_vendor_specific_command_config; // Vendor-specific Admin Command Config
    uint8_t autonomous_power_state_transition;    // Autonomous Power State Transition Support
    uint16_t warning_temperature_threshold;       // Warning Temperature Threshold
    uint16_t critical_temperature_threshold;      // Critical Temperature Threshold
    uint8_t reserved270[242];            // Reserved
    uint8_t submission_queue_entry_size; // Submission Queue Entry Size
    uint8_t completion_queue_entry_size; // Completion Queue Entry Size
    uint8_t reserved514[2];              // Reserved
    uint32_t namespace_count;            // Number of Namespaces
    uint16_t oncs;                       // Optional NVM Command Support
    uint16_t fuses;                      // Fused Operation Support
    uint8_t format_nvm_attributes;       // Format NVM Attributes
    uint8_t volatile_write_cache;        // Volatile Write Cache Support
    uint16_t atomic_write_unit_normal;   // Atomic Write Unit Normal
    uint16_t atomic_write_unit_power_fail; // Atomic Write Unit Power Fail
    uint8_t namespace_capabilities;      // Namespace Capabilities
    uint8_t reserved531;                 // Reserved
    uint16_t atomic_compare_and_write_unit; // Atomic Compare and Write Unit
    uint8_t reserved534[2];              // Reserved
    uint32_t sgls_support;               // Scatter-Gather List Support
    uint8_t reserved540[1508];           // Reserved
    struct nvme_id_power_state power_states[32]; // Power State Descriptors
    uint8_t vendor_specific[1024];       // Vendor Specific
};

// NVMe Controller Optional NVM Command Support Flags
enum {
    NVME_CTRL_ONCS_COMPARE = 1 << 0,              // Compare Command Supported
    NVME_CTRL_ONCS_WRITE_UNCORRECTABLE = 1 << 1, // Write Uncorrectable Command Supported
    NVME_CTRL_ONCS_DSM = 1 << 2,                 // Dataset Management Supported
    NVME_CTRL_VWC_PRESENT = 1 << 0,              // Volatile Write Cache Present
};

// LBA Format Description
struct nvme_lbaf {
    uint16_t metadata_size;        // Metadata Size (bytes)
    uint8_t data_size;             // LBA Data Size (log2 of bytes)
    uint8_t relative_performance;  // Relative Performance Indicator
};

// Namespace Identification Structure
struct nvme_id_ns {
    uint64_t namespace_size;          // Namespace Size (in blocks)
    uint64_t namespace_capacity;      // Namespace Capacity (in blocks)
    uint64_t namespace_utilization;   // Namespace Utilization (in blocks)
    uint8_t namespace_features;       // Namespace Features
    uint8_t num_lba_formats;          // Number of LBA Formats
    uint8_t formatted_lba_size;       // Formatted LBA Size
    uint8_t metadata_capabilities;    // Metadata Capabilities
    uint8_t data_protection_caps;     // Data Protection Capabilities
    uint8_t data_protection_settings; // Data Protection Settings
    uint8_t namespace_multi_path_io_cap; // Multi-path IO and Namespace Sharing Capabilities
    uint8_t reservation_capabilities; // Reservation Capabilities
    uint8_t format_progress_indicator; // Format Progress Indicator
    uint8_t reserved33;               // Reserved
    uint16_t nawun;                   // Namespace Atomic Write Unit Normal
    uint16_t nawupf;                  // Namespace Atomic Write Unit Power Fail
    uint16_t nacwu;                   // Namespace Atomic Compare and Write Unit
    uint16_t nab_sn;                  // Namespace Atomic Boundary Size Normal
    uint16_t nab_o;                   // Namespace Atomic Boundary Offset
    uint16_t nab_spf;                 // Namespace Atomic Boundary Size Power Fail
    uint16_t reserved46;              // Reserved
    uint64_t nvm_capacity[2];         // Namespace NVM Capacity
    uint8_t reserved64[40];           // Reserved
    uint8_t nguid[16];                // Namespace Globally Unique Identifier
    uint8_t eui64[8];                 // IEEE Extended Unique Identifier
    struct nvme_lbaf lba_formats[16]; // LBA Format Support
    uint8_t reserved192[192];         // Reserved
    uint8_t vendor_specific[3712];    // Vendor Specific
};

enum {
    NVME_NS_FLBAS_LBA_MASK = 0xf,
    NVME_NS_FLBAS_META_EXT = 0x10,
    NVME_LBAF_RP_BEST = 0,
    NVME_LBAF_RP_BETTER = 1,
    NVME_LBAF_RP_GOOD = 2,
    NVME_LBAF_RP_DEGRADED = 3,
    NVME_NS_DPC_PI_LAST = 1 << 4,
    NVME_NS_DPC_PI_FIRST = 1 << 3,
    NVME_NS_DPC_PI_TYPE3 = 1 << 2,
    NVME_NS_DPC_PI_TYPE2 = 1 << 1,
    NVME_NS_DPC_PI_TYPE1 = 1 << 0,
    NVME_NS_DPS_PI_FIRST = 1 << 3,
    NVME_NS_DPS_PI_MASK = 0x7,
    NVME_NS_DPS_PI_TYPE1 = 1,
    NVME_NS_DPS_PI_TYPE2 = 2,
    NVME_NS_DPS_PI_TYPE3 = 3,
};

struct nvme_smart_log {
    uint8_t critical_warning;
    uint16_t temperature;
    uint8_t available_spare;
    uint8_t spare_threshold;
    uint8_t percent_used;
    uint8_t reserved6[26];
    uint8_t data_units_read[16];
    uint8_t data_units_written[16];
    uint8_t host_reads[16];
    uint8_t host_writes[16];
    uint8_t controller_busy_time[16];
    uint8_t power_cycles[16];
    uint8_t power_on_hours[16];
    uint8_t unsafe_shutdowns[16];
    uint8_t media_errors[16];
    uint8_t num_error_log_entries[16];
    uint32_t warning_temp_time;
    uint32_t critical_comp_time;
    uint16_t temp_sensor[8];
    uint8_t reserved216[296];
};

enum {
    NVME_SMART_CRIT_SPARE = 1 << 0,
    NVME_SMART_CRIT_TEMPERATURE = 1 << 1,
    NVME_SMART_CRIT_RELIABILITY = 1 << 2,
    NVME_SMART_CRIT_MEDIA = 1 << 3,
    NVME_SMART_CRIT_VOLATILE_MEMORY = 1 << 4,
};

struct nvme_lba_range_type {
    uint8_t type;
    uint8_t attributes;
    uint8_t reserved2[14];
    uint64_t starting_lba;
    uint64_t num_lba;
    uint8_t guid[16];
    uint8_t reserved48[16];
};

enum {
    NVME_LBART_TYPE_FS = 0x01,
    NVME_LBART_TYPE_RAID = 0x02,
    NVME_LBART_TYPE_CACHE = 0x03,
    NVME_LBART_TYPE_SWAP = 0x04,

    NVME_LBART_ATTRIB_TEMP = 1 << 0,
    NVME_LBART_ATTRIB_HIDE = 1 << 1,
};

struct nvme_reservation_status {
    uint32_t generation;
    uint8_t reservation_type;
    uint8_t registered_controller[2];
    uint8_t reserved5[2];
    uint8_t persist_through_power_loss_state;
    uint8_t reserved10[13];
    struct {
        uint16_t controller_id;
        uint8_t registration_status;
        uint8_t reserved3[5];
        uint64_t host_id;
        uint64_t reservation_key;
    } registered_controllers[];
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
    uint16_t control;      // Control flagsvolatile
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


/*
 * Registers should always be accessed with double-word (32-bit) or
 * quad-word (64-bit) accesses. Registers with 64-bit address pointers
 * should be written with double-word accesses by writing the lower
 * double-word (ptr[0]) first, followed by the higher double-word (ptr[1]).
 */
static inline uint64_t nvme_read_q(volatile uint64_t *regs) {
    const volatile uint32_t *ptr = (volatile uint32_t *) regs;
    const uint64_t val_lo = ptr[0];  // Read lower 32 bits
    const uint64_t val_hi = ptr[1];  // Read higher 32 bits

    return val_lo | (val_hi << 32); // Combine to form 64-bit value
}

static inline void nvme_write_q(uint64_t val, volatile uint64_t *regs) {
    volatile uint32_t *ptr = (volatile uint32_t *) regs;
    const uint32_t val_lo = (uint32_t)(val & 0xFFFFFFFF);  // Extract lower 32 bits
    const uint32_t val_hi = (uint32_t)(val >> 32);        // Extract higher 32 bits

    ptr[0] = val_lo;  // Write lower 32 bits first
    ptr[1] = val_hi;  // Write higher 32 bits next
}


// Macros to extract specific fields from the CAP register
#define NVME_CAP_MQES(cap)       ((cap) & 0xFFFF)         // Maximum Queue Entries Supported
#define NVME_CAP_TIMEOUT(cap)    (((cap) >> 24) & 0xFF)   // Timeout in 500ms units
#define NVME_CAP_STRIDE(cap)     (((cap) >> 32) & 0xF)    // Doorbell Stride
#define NVME_CAP_MPSMIN(cap)     (((cap) >> 48) & 0xF)    // Minimum Memory Page Size
#define NVME_CAP_MPSMAX(cap)     (((cap) >> 52) & 0xF)    // Maximum Memory Page Size

// Macro to encode the NVMe version from major and minor numbers
#define NVME_VS(major, minor)    (((major) << 16) | ((minor) << 8))


/*
 * NVMe Base Address Register (BAR) structure.
 * This structure represents the memory-mapped registers of the NVMe controller.
 */

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
}__attribute__((packed));


// Struct representing an NVMe completion queue entry
struct nvme_completion {
    int32_t result;  // Command specific information
    uint32_t reserved;          // Reserved
    int32_t sq_head;           // Submission queue head pointer
    int32_t sq_id;             // Submission queue identifier
    uint16_t command_id;        // Command identifier
    int32_t status;            // Status code
}__attribute__((packed));

// Enum for identifying queue types
enum nvme_queue_id {
    NVME_ADMIN_Q,  // Admin queue
    NVME_IO_Q,     // I/O queue
    NVME_Q_NUM,    // Total number of queues
};

struct nvme_namespace {
    struct doubly_linked_list list;       // Linked list for maintaining a list of namespaces.
    struct nvme_device *device;           // Pointer to the associated NVMe device.
    uint32_t namespace_id;            // Identifier for the namespace.
    uint8_t eui64[8];                     // Extended Unique Identifier (EUI-64) for the namespace.
    int device_number;                    // Device number assigned to the namespace.
    int logical_block_address_shift;      // Shift value for calculating logical block address size.
    uint8_t formatted_lba_size;           // Formatted LBA size and metadata settings.
};
// Struct representing an NVMe queue
struct nvme_queue {
    struct nvme_device *dev;              // Associated NVMe device
    struct nvme_command *submission_queue_commands;      // Submission queue commands
    volatile struct nvme_completion *completion_queue_entries;      // Completion queue entries
    volatile uint32_t *q_db;                    // Doorbell register
    uint16_t q_depth;                  // Queue depth
    int16_t cq_vector;                 // Completion queue interrupt vector
    uint16_t sq_head;                  // Submission queue head pointer
    uint16_t sq_tail;                  // Submission queue tail pointer
    uint16_t cq_head;                  // Completion queue head pointer
    uint16_t qid;                      // Queue identifier
    uint8_t cq_phase;                  // Completion queue phase bit
    uint8_t cqe_seen;                  // Indicates if a CQE was seen
    uint32_t cmdid_data[];        // Command ID data array
}__attribute__((packed));

/* Represents an NVM Express device. Each nvme_dev is a PCI function. */
struct nvme_device {
    struct device *device;             // Pointer to the associated device
    struct doubly_linked_list *node;        // Linked list node for device list
    struct nvme_queue **queues;      // Pointer to an array of NVMe queue pointers
    volatile uint32_t *doorbells;        // Pointer to doorbell registers
    int32_t device_instance;                 // Instance identifier of the device
    unsigned int total_queues;           // Total number of queues
    unsigned int active_queues;          // Number of online queues
    unsigned int max_queue_id;           // Maximum queue identifier
    int queue_depth;                     // Depth of each queue
    uint32_t doorbell_stride;            // Stride between doorbell registers
    uint32_t controller_config;          // Controller configuration
    volatile struct nvme_bar *bar;                // Pointer to Base Address Register (BAR) structure
    struct doubly_linked_list namespaces;     // Linked list of namespaces
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


int32_t nvme_init(struct device *dev, void *other_args);        // Initialize NVMe device
int32_t nvme_shutdown(struct device *dev);    // Shutdown NVMe device
int32_t nvme_scan_namespace();

int32_t nvme_get_namespace_id(struct device *device, uint32_t *namespace_id, uint8_t *extended_unique_identifier);

void setup_nvme_device(struct pci_device *pci_device);
// Helper macros for NVMe queue and data size
#define NVME_ADMIN_QUEUE_SIZE 64                // Admin queue size
#define NVME_IO_QUEUE_SIZE 128                  // I/O queue size
#define NVME_PAGE_SHIFT 12                      // Page shift value (log2 of page size)
#define NVME_PAGE_SIZE (1 << NVME_PAGE_SHIFT)   // Page size
#define NVME_MAX_DATA_SIZE (NVME_PAGE_SIZE * NVME_IO_QUEUE_SIZE) // Maximum data size per I/O queue
#define NVME_LB_SIZE 512                        // Logical Block Size (common value)

#endif // NVME_H_
