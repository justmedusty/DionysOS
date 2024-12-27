//
// Created by dustyn on 8/12/24.
//
#ifndef _DEFINITIONS_H_
#define _DEFINITIONS_H_
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BIT(bit) ((uint8_t)(1 << bit))
#define BYTE(num) (num / 8)


//The ID's I'm going to put below are for spinlock contexts in case there are deadlock/contention issues later
#define SERIAL_LOCK 0
#define SMP_BOOSTRAP_LOCK 1
#define sched_LOCK 2
#define VFS_LOCK 3
#define DIOSFS_LOCK 4 // may need to make multiple but this is fine for now
#define PMM_LOCK 5
#define BTREE_LOCK 6 // there are many of these but it's still an insight
#define BUDDY_LOCK 7
#define RAMDISK_LOCK 8
#define ALLOC_LOCK 9
#define DOUBLY_LINKED_LIST_LOCK 10
#define SINGLE_LINKED_LIST_LOCK 11
#define FRAME_LOCK 12



enum kernel_error_codes {
    KERN_SUCCESS = 0,       // Operation successful
    KERN_BUSY = 16,         // Device or resource busy (EBUSY)
    KERN_NO_MEM = 12,       // Out of memory (ENOMEM)
    KERN_INVALID_ARG = 22,  // Invalid argument (EINVAL)
    KERN_PERMISSION = 13,   // Permission denied (EACCES)
    KERN_NOT_FOUND = 2,     // No such file or directory (ENOENT)
    KERN_EXISTS = 17,       // File exists (EEXIST)
    KERN_IO_ERROR = 5,      // I/O error (EIO)
    KERN_INTERRUPTED = 4,   // Interrupted system call (EINTR)
    KERN_NOT_SUPPORTED = 95 // Operation not supported (ENOTSUP)
};


#define DIONYSOS_ASCII_STRING "                                                                                                                      \n"\
"     _____    ____         _____  _____   ______    _____      _____        ______          _____             ______  \n"\
" ___|\\    \\  |    |   ____|\\    \\|\\    \\ |\\     \\  |\\    \\    /    /|   ___|\\     \\    ____|\\    \\        ___|\\     \\ \n"\
"|    |\\    \\ |    |  /     /\\    \\\\\\    \\| \\     \\ | \\    \\  /    / |  |    |\\     \\  /     /\\    \\      |    |\\     \\\n"\
"|    | |    ||    | /     /  \\    \\\\|    \\  \\     ||  \\____\\/    /  /  |    |/____/| /     /  \\    \\     |    |/____/|\n"\
"|    | |    ||    ||     |    |    ||     \\  |    | \\ |    /    /  /___|    \\|   | ||     |    |    | ___|    \\|   | |\n"\
"|    | |    ||    ||     |    |    ||      \\ |    |  \\|___/    /  /|    \\    \\___|/ |     |    |    ||    \\    \\___|/ \n"\
"|    | |    ||    ||\\     \\  /    /||    |\\ \\|    |      /    /  / |    |\\     \\    |\\     \\  /    /||    |\\     \\    \n"\
"|____|/____/||____|| \\_____\\/____/ ||____||\\_____/|     /____/  /  |\\ ___\\|_____|   | \\_____\\/____/ ||\\ ___\\|_____|   \n"\
"|    /    | ||    | \\ |    ||    | /|    |/ \\|   ||    |`    | /   | |    |     |    \\ |    ||    | /| |    |     |   \n"\
"|____|____|/ |____|  \\|____||____|/ |____|   |___|/    |_____|/     \\|____|_____|     \\|____||____|/  \\|____|_____|   \n"\
"  \\(    )/     \\(       \\(    )/      \\(       )/         )/           \\(    )/          \\(    )/        \\(    )/     \n"\
"   '    '       '        '    '        '       '          '             '    '            '    '          '    '      \n"\
"                                                                                                                  \n"\

#endif
