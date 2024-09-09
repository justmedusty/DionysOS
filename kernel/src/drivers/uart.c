//
// Created by dustyn on 6/19/24.
//
#include "include/types.h"
#include "include/uart.h"

#include <include/data_structures/spinlock.h>
#include "include/definitions.h"
#include "include/arch/x86_64/asm_functions.h"
#include "stdarg.h"


static int is_transmit_empty();
static void write_serial(char a);
static void write_string_serial(const char *str);
static void write_hex_serial(uint64 num,int8 size);
static char get_hex_char(uint8 nibble);

//The serial port and the init serial will need to be IF_DEF'd for multi-arch support later
#define COM1 0x3F8   // COM1 base port

char characters[16] = {'0','1', '2', '3', '4', '5', '6', '7','8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

struct spinlock serial_lock;

void init_serial() {
    outb(COM1 + 1, 0x00);    // Disable all interrupts
    outb(COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1 + 1, 0x00);    //                  (hi byte)
    outb(COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    initlock(&serial_lock,SERIAL_LOCK);
    serial_printf("Serial Initialized\n");
}

static int is_transmit_empty() {
    return inb(COM1 + 5) & 0x20;
}

static void write_serial(char a) {
    while (is_transmit_empty() == 0);
    //correct the serial tomfoolery of just dropping a \n
    if (a == '\n') {
        outb(COM1, '\r');
    }
    outb(COM1, a);
}

//size is the size of the integer so you dont need to print 64 bit ints for a single byte or 4 bytes
static void write_hex_serial(uint64 num, int8 size) {
    write_string_serial("0x");
    for (int8 i = (size - 4); i >= 0; i -= 4) {
        uint8 nibble = (num >> i) & 0xF;  // Extract 4 bits
        write_serial(get_hex_char(nibble));
    }
}


static void write_binary_serial(uint64 num, uint8 size) {
    int separator = 0;
    for (uint8 i = size; i < 0; i--) {
        if (num >> i & 1) {
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

static void write_string_serial(const char *str) {
    while (*str) {
        write_serial(*str++);
    }
}

/*
    Index into the char array if the value is valid
 */
static char get_hex_char(uint8 nibble) {
    if(nibble <= 16) {
        return characters[nibble];
     } else {
    return '?';
     }
}


void serial_printf(char *str, ...) {
    va_list args;
    va_start(args, str);
    acquire_spinlock(&serial_lock);
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
                    char*);
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

    release_spinlock(&serial_lock);
    va_end(args);

}
/*
 * For use in interrupt handlers to avoid any lock contention
 */
void lock_free_serial_printf(char *str, ...) {
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
                    char*);
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