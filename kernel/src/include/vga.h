//
// Created by dustyn on 6/16/24.
//

#ifndef DIONYSOS_VGA_H
#define DIONYSOS_VGA_H
#define VGA_ADDRESS       0xB8000
#define VGA_WIDTH              80
#define VGA_COLOR_BLACK         0
#define VGA_COLOR_BLUE          1
#define VGA_COLOR_GREEN         2
#define VGA_COLOR_CYAN          3
#define VGA_COLOR_RED           4
#define VGA_COLOR_MAGENTA       5
#define VGA_COLOR_BROWN         6
#define VGA_COLOR_LIGHT_GREY    7
#define VGA_COLOR_DARK_GREY     8
#define VGA_COLOR_LIGHT_BLUE    9
#define VGA_COLOR_LIGHT_GREEN   10
#define VGA_COLOR_LIGHT_CYAN    11
#define VGA_COLOR_LIGHT_RED     12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN   14
#define VGA_COLOR_WHITE         15

void write_string( int colour, const char *string );

static inline uint8 vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16 vga_entry(unsigned char uc, uint8 color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

void write_string(const char* str, uint8 color, uint16 x, uint16 y) {
    uint16_t *vga_buffer = (uint16_t *) VGA_ADDRESS;
    for (size_t i = 0; str[i] != '\0'; i++) {
        vga_buffer[y * VGA_WIDTH + x + i] = vga_entry(str[i], color);
    }
}
#endif //DIONYSOS_VGA_H
