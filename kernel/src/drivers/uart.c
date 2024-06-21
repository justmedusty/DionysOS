//
// Created by dustyn on 6/19/24.
//
#include "include/types.h"
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
    if (a == '\n') {
        outb(SERIAL_PORT, '\r');
    }
    outb(SERIAL_PORT, a);
}

void write_hex_serial(uint64 num) {
    int nibble_counter = 0;
    uint8 nibble = 0;
    write_string_serial("0x");
    for (uint64 i = 64; i > 0; i--) {
        if (nibble_counter == 4) {
            nibble_counter = 0;
            write_serial(get_hex_char(nibble));
            nibble = 0;
        }
        if (((num >> i) & 1)) {
            nibble = ((nibble << nibble_counter) | 1);
        } else {
            nibble = ((nibble << nibble_counter) | 0);
        }
        nibble_counter++;
    }

}



void write_binary_serial(uint64 num) {
    int separator = 0;
    for (uint64 i = 0; i < 64; i++) {
        if (((num >> i) & 1)) {
            write_serial('1');
        } else {
            write_serial('0');
        }

        separator++;
        if (separator % 4 == 0 && (i != 63)) {
            write_serial('-');
        }
    }
}

void write_string_serial(const char *str) {
    while (*str) {
        write_serial(*str++);
    }
}

//this is just for early testing and debugging
void bootleg_panic(const char *str) {
    write_string_serial("Panic! ");
    write_string_serial(str);
    for (;;) {
        asm("cli");
        asm("hlt");
    }
}


char get_hex_char(uint8 nibble) {
    switch (nibble) {
        case 0x0:
            return '0';
        case 0x1:
            return '1';
        case 0x2:
            return '2';
        case 0x3:
            return '3';
        case 0x4:
            return '4';
        case 0x5:
            return '5';
        case 0x6:
            return '6';
        case 0x7:
            return '7';
        case 0x8:
            return '8';
        case 0x9:
            return '9';
        case 0xA:
            return 'A';
        case 0xB:
            return 'B';
        case 0xC:
            return 'C';
        case 0xD:
            return 'D';
        case 0xE:
            return 'E';
        case 0xF:
            return 'F';
        default:
            return '?'; // Handle invalid input
    }
}
