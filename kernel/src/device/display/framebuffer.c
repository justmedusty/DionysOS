//
// Created by dustyn on 12/26/24.
//

#include "include/device/display/framebuffer.h"

#include <stdarg.h>
#include <include/data_structures/spinlock.h>
#include <include/definitions/definitions.h>
#include <include/drivers/serial/uart.h>

#include "include/memory/mem.h"
#include "include/device/display/font.h"

struct framebuffer main_framebuffer;
#define MAIN_FB 0

struct framebuffer_ops framebuffer_ops = {
    .clear = fb_ops_clear,
    .draw_string = fb_ops_draw_string,
    .draw_char = fb_ops_draw_char,
    .init = NULL,
    .draw_pixel = NULL
};

struct device_ops framebuffer_device_ops = {
    .framebuffer_ops = &framebuffer_ops,
    .get_status = NULL,
    .configure = NULL,
    .init = NULL,
    .reset = NULL
};


struct device framebuffer_device = {
    .device_major = DEVICE_MAJOR_FRAMEBUFFER,
    .device_minor = MAIN_FB,
    .lock = &main_framebuffer.lock,
    .parent = NULL,
    .device_type = DEVICE_TYPE_BLOCK,
    .device_info = &main_framebuffer,
    .device_ops = &framebuffer_device_ops
};

char characters[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};


void fb_ops_draw_char(struct device *dev, char c, uint32_t color) {
    struct framebuffer *fb = dev->device_info;
    draw_char_with_context(fb, c, color);
};

void fb_ops_draw_string(struct device *dev, uint32_t color, char *s) {
    struct framebuffer *fb = dev->device_info;
    draw_string(fb, s, color);
};

void fb_ops_clear(struct device *dev) {
    struct framebuffer *fb = dev->device_info;
    clear(fb);
}

void draw_char(const struct framebuffer *fb,
               const char c, const uint64_t x, const uint64_t y, const uint32_t color) {
    if (c < 0 || c >= 128) return; // Ensure the character is within the font bounds

    // Access the character's bitmap
    const uint8_t *bitmap = &default_font.data[(uint8_t) c * fb->font_height]; // Each character is 8 bytes tall

    // Loop through each row and column of the character bitmap

    for (uint64_t cy = 0; cy < fb->font_height; cy++) {
        for (uint64_t cx = 0; cx < fb->font_width; cx++) {
            if (bitmap[cy] & BIT(7 - cx)) {
                // Check if the pixel should be set
                const uint64_t px = x + cx; // Calculate absolute X position
                const uint64_t py = y + cy; // Calculate absolute Y position

                // Ensure the pixel is within the framebuffer bounds
                if (px >= 0 && px < fb->width && py >= 0 && py < fb->height) {
                    uint32_t *framebuffer = fb->address;
                    framebuffer[py * (fb->pitch / 4) + px] = color; // Set pixel color
                } else {
                    panic("draw_char: can't draw char");
                }
            }
        }
    }
}

void draw_char_with_context(struct framebuffer *fb,
                            const char c, const uint32_t color) {
    if (c < 0 || c >= 128) return; // Ensure the character is within the font bounds

    if (c == '\n') {
        fb->context.current_y_pos += fb->font_height;
        fb->context.current_x_pos = 0;
        return;
    }

    if (fb->context.current_x_pos >= fb->width) {
        fb->context.current_x_pos = 0;
    }
    if (fb->context.current_y_pos >= fb->height) {
        const uint64_t rows_size = fb->pitch * (fb->height - fb->font_height);
        const uint64_t row_size = fb->pitch * fb->font_height;
        // Shift the framebuffer content upwards by one row
        memmove(fb->address, fb->address + row_size, rows_size);
        // Clear the bottom portion of the framebuffer
        memset(fb->address + rows_size, 0, row_size);
        // Reset the cursor to the last row
        fb->context.current_y_pos = fb->height - fb->font_height;
        fb->context.current_x_pos = 0;
    }

    // Access the character's bitmap
    const uint8_t *bitmap = &default_font.data[(uint8_t) c * fb->font_height]; // Each character is 8 bytes tall

    // Loop through each row and column of the character bitmap
    for (uint64_t cy = 0; cy < fb->font_height; cy++) {
        for (uint64_t cx = 0; cx < fb->font_width; cx++) {
            if (bitmap[cy] & BIT(7 - cx)) {
                // Check if the pixel should be set
                const uint64_t px = fb->context.current_x_pos + cx; // Calculate absolute X position
                const uint64_t py = fb->context.current_y_pos + cy; // Calculate absolute Y position

                // Ensure the pixel is within the framebuffer bounds
                if (px >= 0 && px < fb->width && py >= 0 && py < fb->height) {
                    uint32_t *framebuffer = fb->address;
                    framebuffer[py * (fb->pitch / 4) + px] = color; // Set pixel color
                }
            }
        }
    }
    fb->context.current_x_pos += fb->font_width;
skip:


}

void clear(struct framebuffer *fb) {
    for (uint64_t y = 0; y < (fb->height); y += fb->font_height * fb->pitch) {
        memset((uint32_t *) fb->address + y, 0, fb->width * fb->pitch);
    }
    fb->context.current_x_pos = 0;
    fb->context.current_y_pos = 0;
}

void draw_string(struct framebuffer *fb, const char *str, uint64_t color) {
    uint64_t index_x = fb->context.current_x_pos;
    uint64_t index_y = fb->context.current_y_pos;

    while (*str) {
        if (index_x >= fb->width) {
            index_x = 0;
            index_y += fb->font_height;
        }
        if (index_y >= fb->height) {
            const uint64_t rows_size = fb->pitch * (fb->height - fb->font_height);
            const uint64_t row_size = fb->pitch * fb->font_height;
            // Shift the framebuffer content upwards by one row
            memmove(fb->address, fb->address + row_size, rows_size);
            // Clear the bottom portion of the framebuffer
            memset(fb->address + rows_size, 0, row_size);
            // Reset the cursor to the last row
            index_y = fb->height - fb->font_height;
            index_x = 0;
            continue;
        }

        if (*str == '\n') {
            index_y += fb->font_height;
            index_x = 0;
            str++;
            continue;
        }

        draw_char(fb, *str++, index_x, index_y, color);
        index_x += fb->font_width;
    }

    fb->context.current_x_pos = index_x;
    fb->context.current_y_pos = index_y;
}

static char get_hex_char(uint8_t nibble) {
    if (nibble <= 16) {
        return characters[nibble];
    } else {
        return '?';
    }
}

static void draw_hex(struct device *fb, uint64_t num, int8_t size,uint32_t color) {

    fb->device_ops->framebuffer_ops->draw_string(&framebuffer_device, color, "0x");
    for (int8_t i = (size - 4); i >= 0; i -= 4) {
        uint8_t nibble = (num >> i) & 0xF; // Extract 4 bits
        char c = get_hex_char(nibble);
        fb->device_ops->framebuffer_ops->draw_char(&framebuffer_device, c, color);
    }
}

void kprintf(char *str, ...) {
    va_list args;
    va_start(args, str);
    acquire_spinlock(&main_framebuffer.lock);
    while (*str) {
        if (*str == '\n') {
            framebuffer_device.device_ops->framebuffer_ops->draw_char(&framebuffer_device, '\n', GREEN);
            str++;
            continue;
        }
        if (*str != '%') {
            framebuffer_device.device_ops->framebuffer_ops->draw_char(&framebuffer_device, *str, GREEN);
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
                         */

                        switch (*str) {
                            case '8':
                                uint64_t value8 = va_arg(args, uint32_t);
                                draw_hex(&framebuffer_device,value8, 8,GREEN);
                                break;
                            case '1':
                                uint64_t value16 = va_arg(args, uint32_t);
                                draw_hex(&framebuffer_device,value16, 16,GREEN);

                                str++;
                                break;
                            case '3':
                                uint64_t value32 = va_arg(args, uint32_t);
                                draw_hex(&framebuffer_device,value32, 32,GREEN);

                                str++;
                                break;
                            case '6':
                                uint64_t value64 = va_arg(args, uint64_t);
                                draw_hex(&framebuffer_device,value64, 64,GREEN);

                                str++;
                                break;
                            default:
                                uint64_t value = va_arg(args, uint64_t);
                                draw_hex(&framebuffer_device,value64, 64,GREEN);

                                break;
                        }
                    } else {
                        uint64_t value = va_arg(args, uint64_t);
                        draw_hex(&framebuffer_device,value, 64,GREEN);
                    }
                    break;
                }

                case 's': {
                    char *value = va_arg(args,
                                         char*);
                    framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, GREEN, value);
                    break;
                }

                case 'i': {
                    uint64_t value = va_arg(args, uint64_t);

                    if (value == 0) {
                        framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, GREEN, "0");
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
                        framebuffer_device.device_ops->framebuffer_ops->draw_char(
                            &framebuffer_device, buffer[--index], GREEN);
                    }
                    break;
                }

                default:
                    framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, GREEN, "%");

                    char c = *str;
                    framebuffer_device.device_ops->framebuffer_ops->draw_char(&framebuffer_device, c, GREEN);

                    break;
            }
        }
        str++;
    }

    release_spinlock(&main_framebuffer.lock);
    va_end(args);
}

/*
 * Exception version, lockless in case another thread is holding the lock and all text is RED
 */
void err_printf(char *str, ...) {
    acquire_spinlock(&main_framebuffer.lock);
    va_list args;
    va_start(args, str);
    framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, RED, "[ERROR] ");
    while (*str) {
        if (*str == '\n') {
            framebuffer_device.device_ops->framebuffer_ops->draw_char(&framebuffer_device, '\n', RED);
            str++;
            continue;
        }
        if (*str != '%') {
            framebuffer_device.device_ops->framebuffer_ops->draw_char(&framebuffer_device, *str, RED);
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
                                draw_hex(&framebuffer_device,value8, 8,RED);
                                break;
                            case '1':
                                uint64_t value16 = va_arg(args, uint32_t);
                                draw_hex(&framebuffer_device,value16, 16,RED);

                                str++;
                                break;
                            case '3':
                                uint64_t value32 = va_arg(args, uint32_t);
                                draw_hex(&framebuffer_device,value32, 32,RED);

                                str++;
                                break;
                            case '6':
                                uint64_t value64 = va_arg(args, uint64_t);
                                draw_hex(&framebuffer_device,value64, 64,RED);

                                str++;
                                break;
                            default:
                                uint64_t value = va_arg(args, uint64_t);
                                draw_hex(&framebuffer_device,value64, 64,RED);

                                break;
                        }
                    } else {
                        uint64_t value = va_arg(args, uint64_t);
                        draw_hex(&framebuffer_device,value, 64,RED);
                    }
                    break;
                }

                case 's': {
                    char *value = va_arg(args,
                                         char*);
                    framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, RED, value);
                    break;
                }

                case 'i': {
                    uint64_t value = va_arg(args, uint64_t);

                    if (value == 0) {
                        framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, RED, "0");
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
                        framebuffer_device.device_ops->framebuffer_ops->draw_char(
                            &framebuffer_device, buffer[--index], RED);
                    }
                    break;
                }

                default:
                    framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, RED, "%");

                    char c = *str;
                    framebuffer_device.device_ops->framebuffer_ops->draw_char(&framebuffer_device, c, RED);

                    break;
            }
        }
        str++;
    }

    release_spinlock(&main_framebuffer.lock);
    va_end(args);
}

void warn_printf(char *str, ...) {
    acquire_spinlock(&main_framebuffer.lock);
    va_list args;
    va_start(args, str);
    framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, ORANGE, "[WARN] ");
    while (*str) {
        if (*str == '\n') {
            framebuffer_device.device_ops->framebuffer_ops->draw_char(&framebuffer_device, '\n', ORANGE);
            str++;
            continue;
        }
        if (*str != '%') {
            framebuffer_device.device_ops->framebuffer_ops->draw_char(&framebuffer_device, *str, ORANGE);
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
                                draw_hex(&framebuffer_device,value8, 8,ORANGE);
                                break;
                            case '1':
                                uint64_t value16 = va_arg(args, uint32_t);
                                draw_hex(&framebuffer_device,value16, 16,ORANGE);

                                str++;
                                break;
                            case '3':
                                uint64_t value32 = va_arg(args, uint32_t);
                                draw_hex(&framebuffer_device,value32, 32,ORANGE);

                                str++;
                                break;
                            case '6':
                                uint64_t value64 = va_arg(args, uint64_t);
                                draw_hex(&framebuffer_device,value64, 64,ORANGE);

                                str++;
                                break;
                            default:
                                uint64_t value = va_arg(args, uint64_t);
                                draw_hex(&framebuffer_device,value64, 64,ORANGE);

                                break;
                        }
                    } else {
                        uint64_t value = va_arg(args, uint64_t);
                        draw_hex(&framebuffer_device,value, 64,ORANGE);
                    }
                    break;
                }

                case 's': {
                    char *value = va_arg(args,
                                         char*);
                    framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, ORANGE, value);
                    break;
                }

                case 'i': {
                    uint64_t value = va_arg(args, uint64_t);

                    if (value == 0) {
                        framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, ORANGE, "0");
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
                        framebuffer_device.device_ops->framebuffer_ops->draw_char(
                            &framebuffer_device, buffer[--index], ORANGE);
                    }
                    break;
                }

                default:
                    framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, ORANGE, "%");

                    char c = *str;
                    framebuffer_device.device_ops->framebuffer_ops->draw_char(&framebuffer_device, c, ORANGE);

                    break;
            }
        }
        str++;
    }

    release_spinlock(&main_framebuffer.lock);
    va_end(args);
}

void info_printf(char *str, ...) {
    acquire_spinlock(&main_framebuffer.lock);
    va_list args;
    va_start(args, str);
    framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, YELLOW, "[INFO] ");
    while (*str) {
        if (*str == '\n') {
            framebuffer_device.device_ops->framebuffer_ops->draw_char(&framebuffer_device, '\n', YELLOW);
            str++;
            continue;
        }
        if (*str != '%') {
            framebuffer_device.device_ops->framebuffer_ops->draw_char(&framebuffer_device, *str, YELLOW);
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
                                draw_hex(&framebuffer_device,value8, 8,YELLOW);
                                break;
                            case '1':
                                uint64_t value16 = va_arg(args, uint32_t);
                                draw_hex(&framebuffer_device,value16, 16,YELLOW);

                                str++;
                                break;
                            case '3':
                                uint64_t value32 = va_arg(args, uint32_t);
                                draw_hex(&framebuffer_device,value32, 32,YELLOW);

                                str++;
                                break;
                            case '6':
                                uint64_t value64 = va_arg(args, uint64_t);
                                draw_hex(&framebuffer_device,value64, 64,YELLOW);

                                str++;
                                break;
                            default:
                                uint64_t value = va_arg(args, uint64_t);
                                draw_hex(&framebuffer_device,value64, 64,YELLOW);

                                break;
                        }
                    } else {
                        uint64_t value = va_arg(args, uint64_t);
                        draw_hex(&framebuffer_device,value, 64,YELLOW);
                    }
                    break;
                }

                case 's': {
                    char *value = va_arg(args,
                                         char*);
                    framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, YELLOW, value);
                    break;
                }

                case 'i': {
                    uint64_t value = va_arg(args, uint64_t);

                    if (value == 0) {
                        framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, YELLOW, "0");
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
                        framebuffer_device.device_ops->framebuffer_ops->draw_char(
                            &framebuffer_device, buffer[--index], YELLOW);
                    }
                    break;
                }

                default:
                    framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, YELLOW, "%");

                    char c = *str;
                    framebuffer_device.device_ops->framebuffer_ops->draw_char(&framebuffer_device, c, YELLOW);

                    break;
            }
        }
        str++;
    }

    release_spinlock(&main_framebuffer.lock);
    va_end(args);
}

/*
 * Choose the color
 */
void kprintf_color(uint32_t color, char *str, ...) {
    acquire_spinlock(&main_framebuffer.lock);
    va_list args;
    va_start(args, str);
    while (*str) {
        if (*str == '\n') {
            framebuffer_device.device_ops->framebuffer_ops->draw_char(&framebuffer_device, '\n', color);
            str++;
            continue;
        }
        if (*str != '%') {
            framebuffer_device.device_ops->framebuffer_ops->draw_char(&framebuffer_device, *str, color);
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
                         */

                        switch (*str) {
                            case '8':
                                uint64_t value8 = va_arg(args, uint32_t);
                                draw_hex(&framebuffer_device,value8, 8,color);
                                break;
                            case '1':
                                uint64_t value16 = va_arg(args, uint32_t);
                                draw_hex(&framebuffer_device,value16, 16,color);

                                str++;
                                break;
                            case '3':
                                uint64_t value32 = va_arg(args, uint32_t);
                                draw_hex(&framebuffer_device,value32, 32,color);

                                str++;
                                break;
                            case '6':
                                uint64_t value64 = va_arg(args, uint64_t);
                                draw_hex(&framebuffer_device,value64, 64,color);

                                str++;
                                break;
                            default:
                                uint64_t value = va_arg(args, uint64_t);
                                draw_hex(&framebuffer_device,value64, 64,color);

                                break;
                        }
                    } else {
                        uint64_t value = va_arg(args, uint64_t);
                        draw_hex(&framebuffer_device,value, 64,color);
                    }
                    break;
                }

                case 's': {
                    char *value = va_arg(args,
                                         char*);
                    framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, color, value);
                    break;
                }

                case 'i': {
                    uint64_t value = va_arg(args, uint64_t);

                    if (value == 0) {
                        framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, color, "0");
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
                        framebuffer_device.device_ops->framebuffer_ops->draw_char(
                            &framebuffer_device, buffer[--index], color);
                    }
                    break;
                }

                default:
                    framebuffer_device.device_ops->framebuffer_ops->draw_string(&framebuffer_device, color, "%");

                    char c = *str;
                    framebuffer_device.device_ops->framebuffer_ops->draw_char(&framebuffer_device, c, color);

                    break;
            }
        }
        str++;
    }

    release_spinlock(&main_framebuffer.lock);
    va_end(args);
}


void framebuffer_init() {
    insert_device_into_kernel_tree(&framebuffer_device);
}
