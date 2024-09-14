//
// Created by dustyn on 6/16/24.
//

#include "include/types.h"
#include "include/screen/font.h"
#include "include/screen/draw.h"


void draw_char(uint32 *framebuffer, int32 fb_width, int32 fb_height, int32 fb_pitch,int8 c, int32 x, int32 y, uint32 color) {

    if (c < 0 || c >= 128) return; // Ensure the character is within the font bounds
    const uint8 *bitmap = &default_font.data[(int32)c];
    for (int cy = 0; cy < 8; cy++) { // 8 rows for each character
        for (int cx = 0; cx < 8; cx++) { // 8 columns for each character
            if (bitmap[cy] & (1 << (7 - cx))) { // Check if the pixel should be set
                int px = x + cx;
                int py = y + cy;
                if (px < 0 || px >= fb_width || py < 0 || py >= fb_height) continue; // Ensure within framebuffer bounds
                framebuffer[py * (fb_pitch / 4) + px] = color; // Set pixel color (fb_pitch / 4 because it's in uint32_t units)
            }
        }
    }
}
