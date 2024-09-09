//
// Created by dustyn on 6/19/24.
//

#ifndef KERNEL_UART_H
#define KERNEL_UART_H
void init_serial();
void serial_printf(char *str, ...);
void lock_free_serial_printf(char *str, ...);
#endif //KERNEL_UART_H
