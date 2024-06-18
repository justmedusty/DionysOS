//
// Created by dustyn on 6/17/24.
//

#ifndef DIONYSOS_DRAW_H
#define DIONYSOS_DRAW_H

typedef struct {
    uint16 width;
    uint16 height;
    void *address;
    uint16 pitch;
} draw_context;

typedef uint32 draw_color;

draw_color draw_colors(uint8 r, uint8 g, uint8 b);
draw_color draw_getpixel(draw_context *ctx, int x, int y);
void draw_char(draw_context *ctx, int x, int y, char c, font *font, draw_color color);
void draw_string_simple(draw_context *ctx, int x, int y, char *str, font *font, draw_color color);
void draw_pixel(draw_context *ctx, int x, int y, draw_color color);
void draw_rect(draw_context *ctx, int x, int y, uint16 w, uint16 h, draw_color color);
#endif //DIONYSOS_DRAW_H
