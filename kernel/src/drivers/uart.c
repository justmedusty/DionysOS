//
// Created by dustyn on 6/19/24.
//

#include "include/uart.h"
#include "include/x86.h"

#define SERIAL_PORT 0x3F8   // COM1 base port

void init_serial() {
    outb(SERIAL_PORT + 1, 0x00);    // Disable all interrupts
    outb(SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(SERIAL_PORT + 1, 0x00);    //                  (hi byte)
    outb(SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int is_transmit_empty() {
    return inb(SERIAL_PORT + 5) & 0x20;
}

void write_serial(char a) {
    while (is_transmit_empty() == 0);
    //correct the serial tomfoolery of just dropping a \n
    if(a == '\n'){
        outb(SERIAL_PORT, '\r');
    }
    outb(SERIAL_PORT, a);
}

void write_string_serial(const char *str) {
    while (*str) {
        write_serial(*str++);
    }
}

