//
// Created by dustyn on 6/19/24.
//

#ifndef KERNEL_UART_H
#define KERNEL_UART_H
void init_serial();
int is_transmit_empty();
void write_serial(char a);
void write_string_serial(const char *str);
void write_hex_serial(uint64 num,int8 size);
void bootleg_panic(const char *str);
char get_hex_char(uint8 nibble);
#endif //KERNEL_UART_H
