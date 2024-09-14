//
// Created by dustyn on 6/17/24.
//

#ifndef DIONYSOS_FONT_H
#define DIONYSOS_FONT_H

typedef struct {
    uint32 width;
    uint32 height;
    uint8 *data;
} font;

extern font default_font;

#endif //DIONYSOS_FONT_H
