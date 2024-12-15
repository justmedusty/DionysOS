#pragma once
#include <stdint.h>

typedef struct {
    uint64_t phys_address;
    uint64_t size;
    uint64_t height;
    uint64_t width;
    uint64_t pitch;
} framebuffer_t;

extern framebuffer_t framebuffer;
