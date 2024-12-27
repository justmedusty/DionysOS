//
// Created by dustyn on 12/21/24.
//

#ifndef DIONYSOS_DEVICE_H
#define DIONYSOS_DEVICE_H
#pragma once
#include <stddef.h>
#include "include/device/bus/pci.h"

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

enum {
    DEVICE_TYPE_BLOCK,
    DEVICE_TYPE_CHAR,
    DEVICE_TYPE_NETWORK,
    DEVICE_TYPE_SERIAL
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

    uint64_t (*block_read)(uint64_t byte_offset, size_t bytes_to_read, char *buffer,struct device *device);

    uint64_t (*block_write)(uint64_t byte_offset, size_t bytes_to_write, const char *buffer,struct device *device);

    int32_t (*flush)(struct device *dev);

};

struct char_device_ops {

    int32_t (*put)(char *c,struct device *device);

    int32_t (*get)(char *c,struct device *device);

    int32_t (*ioctl)(struct device *dev, uint32_t cmd, void *arg,struct device *device);

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

    void (*draw_char)(struct device *dev,char c,uint32_t color);

    void (*draw_string)(struct device *dev, uint32_t color, char *string);

    void (*draw_pixel)(struct device *dev,int16_t x, int16_t y, uint16_t color);
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
    };

};


#endif //DIONYSOS_DEVICE_H
