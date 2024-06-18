#pragma once
#include <include/types.h>

typedef struct {
    uint64 phys_address;
    uint64 size;
    uint64 height;
    uint64 width;
    uint64 pitch;
} framebuffer_t;

extern framebuffer_t framebuffer;
