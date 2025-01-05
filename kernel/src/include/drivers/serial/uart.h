//
// Created by dustyn on 6/19/24.
//

#ifndef KERNEL_UART_H
#define KERNEL_UART_H
#define BASE_SERIAL_DEVICE 0
#pragma once
#include "include/device/display/framebuffer.h"
extern struct device serial_device;

struct serial_device {
    struct device *dev;
};

void init_serial();

void serial_printf(char *str, ...);

void lock_free_kprintf(char *str, ...);
#endif //KERNEL_UART_H
