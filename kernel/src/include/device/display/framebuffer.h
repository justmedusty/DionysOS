//
// Created by dustyn on 12/26/24.
//

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H
#include <stdint.h>
#include "include/device/device.h"

extern struct framebuffer main_framebuffer;

enum colors {
    BLACK = 0,
    DARK_BLUE = 0xFFF,
    TEAL = 0xFFFF,
    WHITE = 0xFFFFFFF
};
struct framebuffer_ops {
  void (*init)(void);
  void (*clear)(void);
  void (*draw)(uint64_t x,uint64_t y, char c);
  void (*draw_pixel)(int16_t x, int16_t y, uint16_t color);
  };

 struct framebuffer {
  void *address;
  uint64_t size;
  uint64_t height;
  uint64_t width;
  uint64_t pitch;
  uint64_t font_height;
  uint64_t font_width;

  struct framebuffer_ops *ops;
};

void draw_char(uint32_t *framebuffer, uint64_t fb_width, uint64_t fb_height, uint64_t fb_pitch,
               char c, uint64_t x, uint64_t y, uint32_t color);
void draw_string(struct framebuffer *fb, char *str, uint64_t color);
#endif //FRAMEBUFFER_H
