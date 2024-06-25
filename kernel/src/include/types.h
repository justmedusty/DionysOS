//
// Created by dustyn on 6/16/24.
//

#ifndef DIONYSOS_TYPES_H
#define DIONYSOS_TYPES_H

#define NULL (void *) 0

typedef unsigned int uint32;
typedef int int32;
typedef unsigned short uint16;
typedef short int16;
typedef unsigned char uint8;
typedef char int8;
typedef unsigned long long uint64;
typedef long long int64;

/*For x86_64 paging, when I implement RISC-V in the future this will be a different layout and need to be ifdefd*/
typedef uint64 pte_t;
typedef uint64 pmd_t;
typedef uint64 pud_t;
typedef uint64 p4d_t;
#endif //DIONYSOS_TYPES_H
