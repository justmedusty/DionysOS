//
// Created by dustyn on 6/16/24.
//

#include "include/types.h"
#include "../../include/font.h"
#include "include/draw.h"


static inline void putpixel(draw_context *ctx, uint64 offset, draw_color color) {
    if(offset > ctx->height * ctx->pitch) return;
    *(draw_color *) ((uint64) ctx->address + offset) = color;
}

static inline draw_color getpixel(draw_context *ctx, uint64 offset) {
    if(offset > ctx->height * ctx->pitch) return 0;
    return *(draw_color *) (ctx->address + offset);
}

draw_color draw_colors(uint8 r, uint8 g, uint8 b) {
    return (r << 16) | (g << 8) | (b << 0);
}

draw_color draw_getpixel(draw_context *ctx, int x, int y) {
    return getpixel(ctx, y * ctx->pitch + x * sizeof(draw_color));
}

void draw_char(draw_context *ctx, int x, int y, char c, font *font, draw_color color) {
    int w = font->width;
    int h = font->height;
    if(x < 0) {
        if(w <= -x) return;
        w += x;
        x = 0;
    }
    if(y < 0) {
        if(h <= -y) return;
        h += y;
        y = 0;
    }
    uint8 *font_char = &font->data[((unsigned int) (uint8) c) * font->width * font->height / 8];

    uint64 offset = x * sizeof(draw_color) + y * ctx->pitch;
    for(int yy = 0; yy < h && y + yy < ctx->height; yy++) {
        for(int xx = 0; xx < w && x + xx < ctx->width; xx++) {
            if(font_char[yy] & (1 << (w - xx))) putpixel(ctx, offset + xx * sizeof(draw_color), color);
        }
        offset += ctx->pitch;
    }
}

void draw_string_simple(draw_context *ctx, int x, int y, char *str, font *font, draw_color color) {
    while(*str) {
        draw_char(ctx, x, y, *str++, font, color);
        x += font->width;
    }
}

void draw_pixel(draw_context *ctx, int x, int y, draw_color color) {
    if(x < 0 || y < 0 || x >= ctx->width || y >= ctx->height) return;
    putpixel(ctx, y * ctx->pitch + x * sizeof(draw_color), color);
}

void draw_rect(draw_context *ctx, int x, int y, uint16 w, uint16 h, draw_color color) {
    if(x < 0) {
        if(w <= -x) return;
        w += x;
        x = 0;
    }
    if(y < 0) {
        if(h <= -y) return;
        h += y;
        y = 0;
    }
    if(x + w > ctx->width) {
        w = ctx->width - x;
    }
    if(y + h > ctx->height) {
        h = ctx->height - y;
    }
    uint64 offset = y * ctx->pitch + x * sizeof(draw_color);
    for(int yy = 0; yy < h; yy++) {
        uint64 local_offset = offset;
        for(int xx = 0; xx < w; xx++) {
            putpixel(ctx, local_offset, color);
            local_offset += sizeof(draw_color);
        }
        offset += ctx->pitch;
    }
}
