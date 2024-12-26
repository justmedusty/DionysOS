//
// Created by dustyn on 12/26/24.
//

#include "include/device/display/framebuffer.h"
#include "include/definitions/types.h"
#include "include/device/display/font.h"
#include "include/device/display/draw.h"

struct framebuffer main_framebuffer;

void draw_char(uint32_t *framebuffer, uint64_t fb_width, uint64_t fb_height, uint64_t fb_pitch,
               char c, uint64_t x, uint64_t y, uint32_t color) {
    if (c < 0 || c >= 128) return; // Ensure the character is within the font bounds

    // Access the character's bitmap
    const uint8_t *bitmap = &default_font.data[(uint8_t) c * 16]; // Each character is 8 bytes tall

    // Loop through each row and column of the character bitmap
    for (uint64_t cy = 0; cy < 16; cy++) {
        // 8 rows for each character
        for (uint64_t cx = 0; cx < 16; cx++) {
            // 8 columns for each character
            if (bitmap[cy] & (1 << (7 - cx))) {
                // Check if the pixel should be set
                uint64_t px = x + cx; // Calculate absolute X position
                uint64_t py = y + cy; // Calculate absolute Y position

                // Ensure the pixel is within the framebuffer bounds
                if (px >= 0 && px < fb_width && py >= 0 && py < fb_height) {
                    framebuffer[py * (fb_pitch / 4) + px] = color; // Set pixel color
                }
            }
        }
    }
}

void draw_string(struct framebuffer *fb, char *str, uint64_t color) {
    uint64_t index_x = 0;
    uint64_t index_y = 0;
    uint64_t max_x = fb->width / fb->font_width;
    while (*str) {

        if (*str == '\n') {
            index_y += fb->font_height;
            index_x = 0;
            str++;
            continue;
        }

        if (index_x >= max_x) {
            index_y = 0;
            index_y += fb->font_height;
        }
        draw_char((uint32_t *) fb->address, fb->width, fb->height, fb->pitch, *str++, index_x, index_y, color);
        index_x += fb->font_width;
    }
}
