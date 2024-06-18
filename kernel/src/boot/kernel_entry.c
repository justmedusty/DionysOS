//
// Created by dustyn on 6/17/24.
//
#include "include/framebuffer.h"
#include "include/gdt.h"
#include "include/idt.h"
#include "include/types.h"
#include "include/font.h"

#include "include/draw.h"


void kernel_main(framebuffer_t *framebuffer){

    init_gdt();
    idt_init();

}