//
// Created by dustyn on 6/17/24.
//

#ifndef DIONYSOS_STRING_H
#define DIONYSOS_STRING_H
#include <stddef.h>


void *memcpy(void *dest, const void *src, size_t n) {
    uint8 *pdest = (uint8 *)dest;
    const uint8 *psrc = (const uint8 *)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8 *p = (uint8 *)s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8 *pdest = (uint8 *)dest;
    const uint8 *psrc = (const uint8 *)src;

    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8 *p1 = (const uint8 *)s1;
    const uint8 *p2 = (const uint8 *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}
#endif //DIONYSOS_STRING_H
