//
// Created by dustyn on 6/19/24.
//
#include "include/types.h"
#include "include/uart.h"
#include "include/x86.h"
#include "stdarg.h"
//The serial port and the init serial will need to be IF_DEF'd for multi-arch support later
#define SERIAL_PORT 0x3F8   // COM1 base port

void init_serial() {
    outb(SERIAL_PORT + 1, 0x00);    // Disable all interrupts
    outb(SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(SERIAL_PORT + 1, 0x00);    //                  (hi byte)
    outb(SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    write_string_serial("Serial Initialized\n");
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

//size is the size of the integer so you dont need to print 64 bit ints for a single byte or 4 bytes
void write_hex_serial(uint64 num, int8 size) {
    write_string_serial("0x");
    for (int8 i = (size - 4); i >= 0; i -= 4) {
        uint8 nibble = (num >> i) & 0xF;  // Extract 4 bits
        write_serial(get_hex_char(nibble));
    }
}


void write_binary_serial(uint64 num, uint8 size) {
    int separator = 0;
    for (uint8 i = size; i >= 0; i--) {
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

/*
 * You could also just index into a mapped array if you want to be as clean as possible but for this I am okay with a big switch.
 */
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

void serial_printf(char *str, ...) {
    va_list args;
    va_start(args, str);

    while (*str) {

        if (*str == '\n') {
            write_string_serial("\n");
            str++;
            continue;
        }

        if (*str != '%') {
            write_serial(*str);
        } else {
            str++;
            switch (*str) {
                case 'x': {
                    if (*(str + 1) == '.') {
                        str = str + 2;

                        /*
                         * We will check the length of the hex number, because I am lazy I will only check the first char and skip the second. No need to check the second anyway.
                         * You will still be expected to put the second number there even though the internals do not require it, makes your code more readable anyway.
                         * Usage looks like this :
                         * %x.8 = print 8 bit hex
                         * %x.16 = print 16 bit hex
                         * %x.32 = print 32 bit hex
                         * %x.64 = print 64 bit hex
                         *
                         * Important note, newline character needs to be separated from your x.x by a space..
                         * So like this : x.8 \n
                         * If you do x.8\n
                         * the newline will not work properly.
                         */

                        switch (*str) {
                            case '8':
                                uint64 value8 = va_arg(args, uint32);
                                write_hex_serial(value8, 8);
                                str++;
                                break;
                            case '1':
                                uint64 value16 = va_arg(args, uint32);
                                write_hex_serial(value16, 16);
                                str++;
                                break;
                            case '3' :
                                uint64 value32 = va_arg(args, uint32);
                                write_hex_serial(value32, 32);
                                str++;
                                break;
                            case '6' :
                                uint64 value64 = va_arg(args, uint64);
                                write_hex_serial(value64, 64);
                                str++;
                                break;
                            default:
                                uint64 value = va_arg(args, uint64);
                                write_hex_serial(value, 64);
                                break;
                        }

                    } else {
                        uint64 value = va_arg(args, uint64);
                        write_hex_serial(value, 64);
                    }
                    break;
                }
                case 'b': {
                    uint64 value = va_arg(args, uint64);
                    write_binary_serial(value, 64);
                    break;
                }

                case 's': {
                    char *value = va_arg(args,
                    char);
                    write_string_serial(value);
                    break;
                }

                default:
                    write_serial('%');
                    write_serial(*str);
                    break;
            }
        }
        str++;
    }

    va_end(args);
}