//
// Created by dustyn on 6/17/24.
//

#ifndef DIONYSOS_FONT_H
#define DIONYSOS_FONT_H

#define DEFAULT_FONT_WIDTH 8
#define DEFAULT_FONT_HEIGHT 16
typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t *data;
} font;

extern font default_font;
#endif //DIONYSOS_FONT_H
