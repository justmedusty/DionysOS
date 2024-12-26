//
// Created by dustyn on 12/26/24.
//

#include "include/device/display/framebuffer.h"
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
            if (bitmap[cy] & (1 << (7 - cx))) {
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

void draw_string(struct framebuffer *fb, char *str, uint64_t color) {
    uint64_t index_x = fb->context.current_x_pos;
    uint64_t index_y = fb->context.current_y_pos;;

    while (*str) {
        if (index_x >= fb->width) {
            index_x = 0;
            index_y += fb->font_height;
        }

        if (index_y >= fb->height) {
            uint64_t row_size = fb->width * (fb->height - fb->font_height);

            memmove(fb->address, fb->address + (fb->width * fb->height) - (fb->width * fb->font_height), row_size);

            memset(fb->address + ((fb->height * fb->width) - (fb->width * fb->font_height)), 0, fb->width * fb->font_height);

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
