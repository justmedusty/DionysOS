//
// Created by dustyn on 6/19/24.
//

#ifndef KERNEL_UART_H
#define KERNEL_UART_H
#define BASE_SERIAL_DEVICE 0

extern struct device serial_device;

struct serial_device {
    struct device *dev;
};

void init_serial();

void serial_printf(char *str, ...);

void lock_free_serial_printf(char *str, ...);
#endif //KERNEL_UART_H
