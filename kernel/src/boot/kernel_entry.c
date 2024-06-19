//
// Created by dustyn on 6/17/24.
//
#include "include/framebuffer.h"
#include "include/gdt.h"
#include "include/idt.h"
#include "include/uart.h"
#include "include/types.h"
#include "include/font.h"
#include "include/x86.h"
#include "include/draw.h"


void kernel_main(){
    init_serial();
    write_string_serial("Hello!");
    init_gdt();
    idt_init();

}