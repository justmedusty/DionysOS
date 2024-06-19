//
// Created by dustyn on 6/16/24.
//

#include "include/types.h"
#include <include/gdt.h>
#include "include/x86.h"

extern load_gdt();
struct gdtentry gdtentry[GDT_SIZE];
struct gdtdesc gdtdesc;

void init_gdt_desc(uint32 base, uint32 limit, uint8 access, uint8 flags,
                   struct gdtentry *desc) {
    desc->base0_15 = (base & 0xffff);
    desc->limit0_15 = (limit & 0xffff);
    desc->base24_31 = (base & 0xff000000) >> 24;
    desc->limit16_19 = (limit & 0xf0000) >> 16;
    desc->base16_23 = (base & 0xff0000) >> 16;

    desc->flags = (flags & 0xf);
    desc->access = access;
}

void init_gdt(void) {
    gdtdesc.size = sizeof(struct gdtentry) * GDT_SIZE;
    gdtdesc.offset = (uintptr_t) & gdtentry[0];

    init_gdt_desc(0, 0, 0, 0, &gdtentry[0]);    /* NULL Segment */
    init_gdt_desc(0, 0, PRESENT | SYSTEM | EXECUTABLE | READ_WRITE, PAGE_GR | BITS64, &gdtentry[1]);    // Code segment
    init_gdt_desc(0, 0, PRESENT | SYSTEM | READ_WRITE, PAGE_GR | BITS64, &gdtentry[2]); // Data
    init_gdt_desc(0, 0, PRESENT | SYSTEM | USER_PRIV | EXECUTABLE | READ_WRITE, PAGE_GR | BITS64, &gdtentry[3]);    // User code
    init_gdt_desc(0, 0, PRESENT | SYSTEM | USER_PRIV | READ_WRITE, PAGE_GR | BITS64, &gdtentry[4]); // User data
    //load_gdt(&gdtentry);

}