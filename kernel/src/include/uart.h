//
// Created by dustyn on 6/19/24.
//

#ifndef KERNEL_UART_H
#define KERNEL_UART_H
void init_serial();
int is_transmit_empty();
void write_serial(char a);
void write_string_serial(const char *str);
void write_int_serial(uint64 num);
void bootleg_panic(const char *str);
#endif //KERNEL_UART_H
