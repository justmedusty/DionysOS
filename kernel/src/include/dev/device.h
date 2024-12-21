//
// Created by dustyn on 12/21/24.
//

#ifndef DIONYSOS_DEVICE_H
#define DIONYSOS_DEVICE_H
#pragma once

#include "include/dev/bus/pci.h"

enum {
    DEVICE_MAJOR_RAMDISK,
    DEVICE_MAJOR_FRAMEBUFFER,
    DEVICE_MAJOR_NETWORK_CARD,
    DEVICE_MAJOR_SSD,
    DEVICE_MAJOR_HARD_DISK,
    DEVICE_MAJOR_KEYBOARD,
    DEVICE_MAJOR_MOUSE,
    DEVICE_MAJOR_SERIAL,
    DEVICE_MAJOR_USB_CONTROLLER,
    DEVICE_MAJOR_WIFI_ADAPTER
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
};

//not sure if I will use this yet
struct device_driver {
    struct *device device;
    struct pci_driver *pci_driver;
    struct device_ops *device_ops;
};

struct device_ops {
    int32_t (*init)(struct device *dev);

    int32_t (*shutdown)(struct device *dev);

    int32_t (*reset)(struct device *dev);

    int32_t (*get_status)(struct device *dev);

    int32_t (*configure)(struct device *dev, void *args);

    union {
        struct block_device_ops block_device_ops;
        struct char_device_ops char_device_ops;
        struct network_device_ops network_device_ops;
        struct pci_device_ops pci_device_ops;
    };
};


struct block_device_ops {
    int64_t (*block_read)(uint64_t byte_offset, size_t bytes_to_read, char *buffer);

    int64_t (*block_write)(uint64_t byte_offset, size_t bytes_to_read, char *buffer);

    int32_t (*flush)(struct device *dev);
};

struct char_device_ops {
    int32_t (*put)(char *c);

    int32_t (*get)(char *c);

    int32_t (*write)(const char *buffer, size_t size);

    int32_t (*read)(char *buffer, size_t size);

    int32_t (*ioctl)(struct device *dev, uint32_t cmd, void *arg);
};

struct network_device_ops {
    int32_t (*send_packet)(const char *packet, size_t length);

    int32_t (*receive_packet)(char *buffer, size_t buffer_size);

    int32_t (*set_mac_address)(struct device *dev, const char *mac_address);

    int32_t (*get_mac_address)(struct device *dev, char *mac_address);

    int32_t (*set_ip_address)(struct device *dev, const char *ip_address);

    int32_t (*get_ip_address)(struct device *dev, char *ip_address);
};


#endif //DIONYSOS_DEVICE_H
