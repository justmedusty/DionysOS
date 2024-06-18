//
// Created by dustyn on 6/17/24.
//

#include "include/gdt.h"
#include "include/idt.h"
#include "include/types.h"
#include "include/font.h"
#include "include/draw.h"

int main(void *framebuffer) {

    init_gdt();
    idt_init();
    return 0;
}