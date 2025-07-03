//
// Created by dustyn on 12/28/24.
//
#include "include/device/device.h"

#include <include/data_structures/binary_tree.h>
#include <include/data_structures/doubly_linked_list.h>
#include <include/definitions/string.h>
#include <include/drivers/display/framebuffer.h>
#include <include/drivers/serial/uart.h>

struct binary_tree system_device_tree;

uint64_t device_minor_map[NUM_DEVICE_MAJOR_CLASSIFICATIONS] = {0};

const char *device_major_strings[NUM_DEVICE_MAJOR_CLASSIFICATIONS] = {
        [DEVICE_MAJOR_NETWORK_CARD] = "NETWORK_CARD",
        [DEVICE_MAJOR_NVME] = "NVME",
        [DEVICE_MAJOR_RAMDISK] = "RAMDISK",
        [DEVICE_MAJOR_AHCI] = "AHCI",
        [DEVICE_MAJOR_KEYBOARD] = "KEYBOARD",
        [DEVICE_MAJOR_FRAMEBUFFER] = "FRAMEBUFFER",
        [DEVICE_MAJOR_MOUSE] = "MOUSE",
        [DEVICE_MAJOR_SERIAL] = "SERIAL",
        [DEVICE_MAJOR_USB_CONTROLLER] = "USB_CONTROLLER",
        [DEVICE_MAJOR_WIFI_ADAPTER] = "WIFI_ADAPTER",
        [DEVICE_MAJOR_TMPFS] = "TMPFS"
};

char *device_minor_to_string(uint64_t minor) {
    if (minor > 20) {
        return NULL;
    }
    char string[16];
    uint64_t i = 0;
    while (minor > 0) {
        string[i++] = '0' + (minor % 10);
        minor /= 10;
    }
    char *result = kzmalloc(strlen(string) + 1);

    const uint64_t j = i;
    while (i--) {
        result[j - i] = string[i];
    }

    result[j] = '\0';

    return result;

}
char *get_device_node_name(struct device *device) {
    char *string = kzmalloc(64);
    strcpy(string,(char *) device_major_strings[device->device_major]);
    char *minor = device_minor_to_string(device->device_minor);
    strcpy(string + strlen(string), minor);
    kfree(minor);
    string[strlen(string)] = '\0';
}
static struct device_group *get_device_group(uint64_t device_major);

void insert_device_into_device_group(struct device *device, struct device_group *device_group);

struct device_group *alloc_new_device_group(uint64_t device_major);

void init_system_device_tree() {
    init_tree(&system_device_tree, REGULAR_TREE, 0);
    serial_printf("System device tree created\n");
    kprintf("System device tree created\n");
}


void insert_device_into_kernel_tree(struct device *device) {
    const uint64_t device_major = device->device_major;
    if (device_major > NUM_DEVICE_MAJOR_CLASSIFICATIONS) {
        warn_printf("Unknown device major number %i\n", device_major);
        return;
    }
    struct device_group *device_group = get_device_group(device->device_major);
    if (device_group == NULL) {
        device_group = alloc_new_device_group(device_major);
    }

    insert_device_into_device_group(device, device_group);
}

static struct device_group *get_device_group(const uint64_t device_major) {
    struct device_group *group = lookup_tree(&system_device_tree, device_major, false);

    if (group && group->device_major == device_major) {
        return group;
    }

    return NULL;
}

struct device_group *alloc_new_device_group(uint64_t device_major) {
    struct device_group *device_group = kmalloc(sizeof(struct device_group));
    device_group->device_major = device_major;
    device_group->devices = kmalloc(DEVICE_GROUP_SIZE * sizeof(uintptr_t));
    device_group->num_devices = 0;
    device_group->name = device_major_strings[device_major];
    kprintf_color(CYAN, "Created device group for device type %s\n", device_group->name);
    insert_tree_node(&system_device_tree, device_group, device_major);
    return device_group;
}

void insert_device_into_device_group(struct device *device, struct device_group *device_group) {
    if (device_group->num_devices >= DEVICE_GROUP_SIZE) {
        serial_printf("[ERROR] Too many devices in device group %s\n", device_group->name);
        return;
    }

    device_group->devices[device_group->num_devices++] = device;
}

struct device *query_device(const uint64_t device_major, const uint64_t device_minor) {
    struct device_group *device_group = get_device_group(device_major);
    if (device_group == NULL) {
        serial_printf("[ERROR] Device group does not exist (%i)\n", device_major);
        return NULL;
    }
    size_t index = 0;
    struct device *current = device_group->devices[index];

    while (index < device_group->num_devices) {
        if (device_group->devices[index]->device_minor == device_minor) {
            return current;
        }
        index++;
    }

    serial_printf("[ERROR] Device does not exist (%i:%i)\n", device_major, device_minor);
    return NULL;
}

void create_device(struct device *device, uint64_t device_major, char *name, struct device_ops *device_ops,
                   void *device_info, struct device *parent) {
    if (device_major > NUM_DEVICE_MAJOR_CLASSIFICATIONS) {
        /*
         * This is an extremist response but that is fine because it shouldnt ever happen anyway and if something causes it
         * to happen I want visibility above all else, later I can change this to a kernel log call.
         */
        panic("Bad device major passed");
    }
    device->device_major = device_major;
    safe_strcpy(device->name, name, DEVICE_NAME_LEN);
    device->name[DEVICE_NAME_LEN + 1] = '\0'; // just in case there was any fuckery going on with the passed buffer
    device->driver->device_ops = device_ops;
    device->device_minor = device_minor_map[device_major]++;
    device->device_info = device_info;
    device->parent = parent;
}