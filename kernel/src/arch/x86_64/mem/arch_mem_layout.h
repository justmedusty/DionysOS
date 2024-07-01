//
// Created by dustyn on 6/26/24.
//

#ifndef DIONYSOS_ARCH_MEM_LAYOUT_H
#define DIONYSOS_ARCH_MEM_LAYOUT_H
#define KERNBASE 0xffffffff00000000
#define EXTMEM   0x80000000          // Start of extended memory
#define DEVSPACE 0xFE000000         // Other devices are at high addresses

// Key addresses for address space layout (see kmap in vm.c for layout)
#define KERNLINK (KERNBASE+EXTMEM)  // Address where kernel is linked
#endif //DIONYSOS_ARCH_MEM_LAYOUT_H
