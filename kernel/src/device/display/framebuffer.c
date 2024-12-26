//
// Created by dustyn on 12/26/24.
//

#include "include/device/display/framebuffer.h"

#include <include/definitions/definitions.h>

#include "include/memory/mem.h"
#include "include/device/display/font.h"

struct framebuffer main_framebuffer;

struct device framebuffer_device = {0};

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
                }
            }
        }
    }
}

void draw_char_with_context(struct framebuffer *fb,
               const char c, const uint32_t color) {
    if (c < 0 || c >= 128) return; // Ensure the character is within the font bounds

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
    fb->context.current_y_pos += fb->font_height;
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
