//
// Created by dustyn on 9/17/24.
//

#ifndef _NVME_H_
#define _NVME_H_

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
    NVME_COPY = 0x19,
    NVME_GET_LOG_PAGE = 0x06,
    NVME_GET_LOG_PAGE_SIZE = 0x02,
    NVME_IDENTIFY = 0x06,
    NVME_ASYNC_EVENT_CONFIGURATION = 0x01,
    NVME_GET_LOG_PAGE_SIZE_EXT = 0x03
};

// NVMe Status Codes
enum nvme_status_codes {
    NVME_SC_SUCCESS = 0x00,
    NVME_SC_INVALID_OPCODE = 0x01,
    NVME_SC_INVALID_FIELD = 0x02,
    NVME_SC_COMMAND_ID_CONFLICT = 0x03,
    NVME_SC_DATA_TRANSFER_ERROR = 0x04,
    NVME_SC_ABORTED_POWER_LOSS = 0x05,
    NVME_SC_INTERNAL_DEVICE_ERROR = 0x06,
    NVME_SC_ABORTED_BY_REQUEST = 0x07,
    NVME_SC_ABORTED_SQ_DELETION = 0x08,
    NVME_SC_ABORTED_FAILED_FUSED = 0x09,
    NVME_SC_ABORTED_MISSING_FUSED = 0x0A,
    NVME_SC_INVALID_NAMESPACE_OR_FORMAT = 0x0B,
    NVME_SC_COMMAND_SEQUENCE_ERROR = 0x0C,
    NVME_SC_INVALID_SGL_SEGMENT_DESCRIPTOR = 0x0D,
    NVME_SC_INVALID_SGL_DESCRIPTOR_TYPE = 0x0E,
    NVME_SC_INVALID_USE_OF_CMB = 0x0F,
    NVME_SC_PRP_OFFSET_INVALID = 0x10,
    NVME_SC_ATOMIC_WRITE_UNIT_EXCEEDED = 0x14,
    NVME_SC_OPERATION_DENIED = 0x12,
    NVME_SC_SGL_OFFSET_INVALID = 0x13,
    NVME_SC_INVALID_CONTROLLER_MEM_BUFFER = 0x14,
    NVME_SC_INVALID_COMMAND_SIZE = 0x15,
    NVME_SC_ABORTED_COMMAND_IN_PROGRESS = 0x16,
    NVME_SC_ABORTED_COMMAND_QUEUE_ERROR = 0x17
};

// NVMe Command structure
struct nvme_command {
    uint8_t opcode;
    uint8_t flags;
    uint16_t command_id;
    uint32_t namespace_id;
    uint64_t nlb; // Number of LBAs
    uint64_t slba; // Starting LBA
    uint32_t metadata_pointer;
    uint32_t data_pointer;
    uint32_t data_length;
    uint32_t control;
    uint32_t dsmgmt;
    uint32_t reserved2[5];
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

// Helper macros (adjust as needed)
#define NVME_ADMIN_QUEUE_SIZE 64
#define NVME_IO_QUEUE_SIZE 128
#define NVME_PAGE_SHIFT 12
#define NVME_PAGE_SIZE (1 << NVME_PAGE_SHIFT)
#define NVME_MAX_DATA_SIZE (NVME_PAGE_SIZE * NVME_IO_QUEUE_SIZE)

#endif // NVME_H
