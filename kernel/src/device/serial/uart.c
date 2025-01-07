//
// Created by dustyn on 6/19/24.
//
#include "include/definitions/types.h"
#include "include/drivers/serial/uart.h"

#include <include/data_structures/spinlock.h>
#include "include/definitions/definitions.h"
#include "include/architecture/generic_asm_functions.h"
#include "stdarg.h"


static int is_transmit_empty();

static void write_serial(char a);

static void write_string_serial(const char *str);

static void write_hex_serial(uint64_t num, int8_t size);

static char get_hex_char(uint8_t nibble);

#ifdef __x86_64__
#define BASE 0x3F8   // BASE port
#endif

struct spinlock serial_lock;
struct device serial_device = {0};
/*
 * Your typical UART setup, BASE port
 */
int32_t put(char *c, struct device *device);

uint32_t get(uint32_t port, struct device *device);

struct char_device_ops serial_ops = {
    .get = get,
    .put = put,
    .ioctl = NULL
};

struct device_ops main_serial_ops = {
    .init = NULL,
    .shutdown = NULL,
    .reset = NULL,
    .char_device_ops = &serial_ops
};


void init_serial() {


#ifdef __x86_64__
    write_port(BASE + 1, 0x00); // Disable all interrupts
    write_port(BASE + 3, 0x80); // Enable DLAB (set baud rate divisor)
    write_port(BASE + 0, 0x03); // St divisor to 3 (lo byte) 38400 baud
    write_port(BASE + 1, 0x00); //                  (hi byte)
    write_port(BASE + 3, 0x03); // 8 bits, no parity, one stop bit
    write_port(BASE + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    write_port(BASE + 4, 0x0B); // IRQs enabled, RTS/DSR set
#endif
    initlock(&serial_lock,SERIAL_LOCK);
    serial_device.device_major = DEVICE_MAJOR_SERIAL;
    serial_device.device_minor = BASE_SERIAL_DEVICE;
    serial_device.lock = &serial_lock;
    serial_device.parent = NULL;
    serial_device.device_ops = &main_serial_ops;
    serial_device.uses_dma = false;
    serial_device.device_info = NULL;
    serial_device.pci_driver = NULL;
    serial_device.device_type = DEVICE_TYPE_CHAR;
    kprintf("Serial Initialized\n");
}

static int is_transmit_empty() {
#ifdef __x86_64__
    int32_t ret = read_port(BASE + 5) & 0x20;
#endif
    return ret;
}

int32_t put(char *c, struct device *device) {
    write_port(BASE, *c);
    return KERN_SUCCESS;
}

uint32_t get(uint32_t port, struct device *device) {
    return read_port(BASE);
}

/*
 *  This function writes a single byte to the serial port,
 *  if it is a linebreak, it writes a carriage return after
 *  because this is required to actually break the line
 */
static void write_serial(char a) {
    while (is_transmit_empty() == 0);
    //correct the serial tomfoolery of just dropping a \n
    if (a == '\n') {
        write_port(BASE, '\r');
    }
    write_port(BASE, a);
}

/*
 * Writes a hex number of size (ie 8 bit, 16 bit, 32bit, 64 bit)
 */
static void write_hex_serial(uint64_t num, int8_t size) {
    write_string_serial("0x");
    for (int8_t i = (size - 4); i >= 0; i -= 4) {
        uint8_t nibble = (num >> i) & 0xF; // Extract 4 bits
        write_serial(get_hex_char(nibble));
    }
}

/*
 * Unused but writes binary numbers of size
 */
static void write_binary_serial(uint64_t num, uint8_t size) {
    int separator = 0;
    for (uint8_t i = size; i < 0; i--) {
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

/*
 * Writes string to serial, just calls write_serial repeatedly
 */
static void write_string_serial(const char *str) {
    while (*str) {
        write_serial(*str++);
    }
}

/*
    Index into the char array if the value is valid
 */
static char get_hex_char(uint8_t nibble) {
    if (nibble <= 16) {
        return characters[nibble];
    } else {
        return '?';
    }
}

/*
 * This is my homegrown printf, it allows printing unsigned integers, hex integers, strings, and binary\
 *
 * if I find it necessary later I'll add ones compliment support
 */
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
                         */

                        switch (*str) {
                            case '8':
                                uint64_t value8 = va_arg(args, uint32_t);
                                write_hex_serial(value8, 8);
                                break;
                            case '1':
                                uint64_t value16 = va_arg(args, uint32_t);
                                write_hex_serial(value16, 16);
                                str++;
                                break;
                            case '3':
                                uint64_t value32 = va_arg(args, uint32_t);
                                write_hex_serial(value32, 32);
                                str++;
                                break;
                            case '6':
                                uint64_t value64 = va_arg(args, uint64_t);
                                write_hex_serial(value64, 64);
                                str++;
                                break;
                            default:
                                uint64_t value = va_arg(args, uint64_t);
                                write_hex_serial(value, 64);
                                break;
                        }
                    } else {
                        uint64_t value = va_arg(args, uint64_t);
                        write_hex_serial(value, 64);
                    }
                    break;
                }
                case 'b': {
                    uint64_t value = va_arg(args, uint64_t);
                    write_binary_serial(value, 64);
                    break;
                }

                case 's': {
                    char *value = va_arg(args,
                                         char*);
                    write_string_serial(value);
                    break;
                }

                case 'i': {
                    uint64_t value = va_arg(args, uint64_t);

                    if (value == 0) {
                        write_serial('0');
                        break;
                    }

                    char buffer[20]; // Enough to hold the maximum 64 bit value
                    int index = 0;

                    while (value > 0) {
                        buffer[index++] = characters[value % 10];
                        value /= 10;
                    }

                    // Write digits in reverse order
                    while (index > 0) {
                        write_serial(buffer[--index]);
                    }
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
