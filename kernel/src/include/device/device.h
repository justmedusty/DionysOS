//
// Created by dustyn on 12/21/24.
//

#ifndef DIONYSOS_DEVICE_H
#define DIONYSOS_DEVICE_H
#pragma once

#include <stddef.h>
#include "include/drivers/bus/pci.h"

#define DEVICE_GROUP_SIZE 32
#define DEVICE_NAME_LEN 30
#define NUM_DEVICE_MAJOR_CLASSIFICATIONS 11

enum major {
    DEVICE_MAJOR_RAMDISK,
    DEVICE_MAJOR_FRAMEBUFFER,
    DEVICE_MAJOR_NETWORK_CARD,
    DEVICE_MAJOR_NVME,
    DEVICE_MAJOR_AHCI,
    DEVICE_MAJOR_KEYBOARD,
    DEVICE_MAJOR_MOUSE,
    DEVICE_MAJOR_SERIAL,
    DEVICE_MAJOR_USB_CONTROLLER,
    DEVICE_MAJOR_WIFI_ADAPTER,
    DEVICE_MAJOR_TMPFS
};

extern const char *device_major_strings[NUM_DEVICE_MAJOR_CLASSIFICATIONS];

enum {
    DEVICE_TYPE_BLOCK,
    DEVICE_TYPE_CHAR,
    DEVICE_TYPE_NETWORK,
    DEVICE_TYPE_SERIAL
};

struct device_group {
    const char *name;
    uint64_t device_major;
    uint64_t num_devices;
    struct device **devices;
};

struct device {
    struct device *parent;
    struct spinlock *lock;
    uint64_t device_major;
    uint64_t device_minor;
    uint64_t device_class;
    uint64_t device_type;
    char name[32];
    bool uses_dma;
    struct device_ops *device_ops;
    struct pci_driver *pci_driver;
    void *device_info;
};

//not sure if I will use this yet
struct device_driver {
    struct device *device;
    struct pci_driver *pci_driver;
    struct device_ops *device_ops;
};

struct block_device_ops {
    uint64_t (*block_read)(uint64_t block_number, size_t block_count, char *buffer, struct device *device);

    uint64_t (*block_write)(uint64_t block_number, size_t block_count, char *buffer, struct device *device);

    int32_t (*flush)(struct device *dev);
};

struct char_device_ops {
    int32_t (*put)(char *c, struct device *device);

    uint32_t (*get)(uint32_t port, struct device *device);

    int32_t (*ioctl)(struct device *dev, uint32_t cmd, void *arg, struct device *device);
};

struct network_device_ops {
    int32_t (*send_packet)(const char *packet, size_t length);

    int32_t (*receive_packet)(char *buffer, size_t buffer_size);

    int32_t (*set_mac_address)(struct device *dev, const char *mac_address);

    int32_t (*get_mac_address)(struct device *dev, char *mac_address);

    int32_t (*set_ip_address)(struct device *dev, const char *ip_address);

    int32_t (*get_ip_address)(struct device *dev, char *ip_address);
};

struct framebuffer_ops {
    void (*init)(void);

    void (*clear)(struct device *dev);

    void (*draw_char)(struct device *dev, uint8_t c, uint32_t color);

    void (*draw_string)(struct device *dev, uint32_t color, char *string);

    void (*draw_pixel)(struct device *dev, int16_t x, int16_t y, uint16_t color);
};

struct device_ops {
    int32_t (*init)(struct device *dev, void *extra_arguments);

    int32_t (*shutdown)(struct device *dev);

    int32_t (*reset)(struct device *dev);

    int32_t (*get_status)(struct device *dev);

    int32_t (*configure)(struct device *dev, void *args);

    union {
        struct block_device_ops *block_device_ops;

        struct char_device_ops *char_device_ops;

        struct network_device_ops *network_device_ops;

        struct framebuffer_ops *framebuffer_ops;

        struct nvme_ops *nvme_ops;
    };
};

void init_system_device_tree();

void create_device(struct device *device, uint64_t device_major, char *name, struct device_ops *device_ops,
                   void *device_info, struct device *parent);

void insert_device_into_kernel_tree(struct device *device);

struct device *query_device(uint64_t device_major, uint64_t device_minor);

#endif //DIONYSOS_DEVICE_H
