//
// Created by dustyn on 12/26/24.
//

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H
#include <stdint.h>
#include <include/data_structures/spinlock.h>

#include "include/device/device.h"

extern struct framebuffer main_framebuffer;

extern char characters[16];

enum colors {
    // Basic colors
    BLACK = 0x000000, // Black
    WHITE = 0xFFFFFF, // White
    RED = 0xFF0000, // Red
    GREEN = 0x00FF00, // Green
    BLUE = 0x0000FF, // Blue
    YELLOW = 0xFFFF00, // Yellow
    CYAN = 0x00FFFF, // Cyan
    MAGENTA = 0xFF00FF, // Magenta

    // Shades of gray
    GRAY = 0x808080, // Gray
    DARK_GRAY = 0x404040, // Dark Gray
    LIGHT_GRAY = 0xD3D3D3, // Light Gray

    // Primary colors
    DARK_BLUE = 0x00008B, // Dark Blue
    DARK_GREEN = 0x006400, // Dark Green
    DARK_RED = 0x8B0000, // Dark Red
    LIGHT_BLUE = 0xADD8E6, // Light Blue
    LIGHT_GREEN = 0x90EE90, // Light Green
    LIGHT_RED = 0xF08080, // Light Red

    // Other colors
    TEAL = 0x008080, // Teal
    OLIVE = 0x808000, // Olive
    PURPLE = 0x800080, // Purple
    BROWN = 0xA52A2A, // Brown
    PINK = 0xFFC0CB, // Pink
    ORANGE = 0xFFA500, // Orange

    // Pastel colors
    PASTEL_BLUE = 0xAEC6CF, // Pastel Blue
    PASTEL_GREEN = 0x77DD77, // Pastel Green
    PASTEL_PURPLE = 0xB39EB5, // Pastel Purple
    PASTEL_YELLOW = 0xFDFD96, // Pastel Yellow

    // Shades of specific colors
    LIGHT_TEAL = 0x20B2AA, // Light Teal
    DARK_TEAL = 0x008B8B, // Dark Teal
    LIGHT_OLIVE = 0xBDB76B, // Light Olive
    DARK_OLIVE = 0x556B2F, // Dark Olive
    LIGHT_PURPLE = 0xDDA0DD, // Light Purple
    DARK_PURPLE = 0x6A0DAD, // Dark Purple
    LIGHT_BROWN = 0xD2B48C, // Light Brown
    DARK_BROWN = 0x8B4513, // Dark Brown

    // Other interesting colors
    GOLD = 0xFFD700, // Gold
    SILVER = 0xC0C0C0, // Silver
    BRONZE = 0xCD7F32, // Bronze
    IVORY = 0xFFFFF0, // Ivory
    BEIGE = 0xF5F5DC, // Beige
    LAVENDER = 0xE6E6FA, // Lavender
    SALMON = 0xFA8072, // Salmon
    CHOCOLATE = 0xD2691E, // Chocolate
    CORAL = 0xFF7F50, // Coral
};

struct framebuffer_ops {
    void (*init)(void);

    void (*clear)(void);

    void (*draw_char)(struct framebuffer *fb, uint64_t x, uint64_t y, char c);

    void (*draw_string)(struct framebuffer *fb, uint64_t x, uint64_t y, char *string);

    void (*draw_pixel)(int16_t x, int16_t y, uint16_t color);
};

struct text_mode_context {
    uint64_t current_x_pos;
    uint64_t current_y_pos;
};

struct framebuffer {
    void *address;
    uint64_t size;
    uint64_t height;
    uint64_t width;
    uint64_t pitch;
    uint64_t font_height;
    uint64_t font_width;
    struct text_mode_context context;
    struct framebuffer_ops *ops;
    struct spinlock lock;
};


void draw_char(const struct framebuffer *fb,
               char c, uint64_t x, uint64_t y, uint32_t color);

void draw_char_with_context(struct framebuffer *fb,
                            const char c, const uint32_t color);

void draw_string(struct framebuffer *fb, const char *str, uint64_t color);
void kprintf(char *str, ...);
void kprintf_color(uint32_t color,char *str, ...);
void kprintf_exception(char *str, ...);
#endif //FRAMEBUFFER_H
