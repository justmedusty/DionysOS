//
// Created by dustyn on 8/12/24.
//
#ifndef _DEFINITIONS_H_
#define _DEFINITIONS_H_
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <include/memory/pmm.h>


#ifdef _DEBUG_
#define DEBUG_PRINT(string, ...) debug_printf(string, ## __VA_ARGS__); serial_printf(string, ## __VA_ARGS__);
#else
#define DEBUG_PRINT(string, ...)
#endif

#define USER_STACK_TOP 0x7FFFFFFFF000ULL
#define KERNEL_BASE  0xFFFFFFFF80000000ULL
#define KERNEL_SIZE (4 << 30)
#define STACK_SIZE   0x2000ULL     // 16 KiB

#define KERNEL_STACK_BOTTOM (KERNEL_BASE + KERNEL_SIZE + 0x10000)
#define KERNEL_STACK_TOP    (KERNEL_STACK_BOTTOM + STACK_SIZE)


typedef unsigned char byte;

#define BIT(bit) ((1UL << bit))
#define BYTE(num) (num / 8)


extern struct vnode *procfs_root;
extern struct vnode *kernel_message;
extern bool procfs_online;

typedef void (*worker_function)(void *args);

//The ID's I'm going to put below are for spinlock contexts in case there are deadlock/contention issues later
enum {
    SERIAL_LOCK,
    SMP_BOOSTRAP_LOCK,
    SCHED_LOCK,
    VFS_LOCK,
    DIOSFS_LOCK, // may need to make multiple but this is fine for now
    PMM_LOCK,
    BTREE_LOCK, // there are many of these but it's still an insight
    BUDDY_LOCK,
    RAMDISK_LOCK,
    ALLOC_LOCK,
    DOUBLY_LINKED_LIST_LOCK,
    SINGLE_LINKED_LIST_LOCK,
    FRAME_LOCK,
    QUEUE_LOCK,
    KERNEL_MESSAGE_LOCK,
};

#define SPRINTF_MAX_LEN 4096

enum seek_values {
    SEEK_BEGIN = 0xFFFFFFFFFFFFFFFF,
    SEEK_CUR = 0xFFFFFFFFFFFFFFFE,
    SEEK_END = 0xFFFFFFFFFFFFFFFD,
};


enum kernel_error_codes {
// General Success and Error Codes
    KERN_SUCCESS = 0,              // Operation successful
    KERN_BUSY = -1,               // Device or resource busy (EBUSY)
    KERN_NO_MEM = -2,             // Out of memory (ENOMEM)
    KERN_INVALID_ARG = -3,        // Invalid argument (EINVAL)
    KERN_PERMISSION = -4,         // Permission denied (EACCES)
    KERN_NOT_FOUND = -5,          // No such file or directory (ENOENT)
    KERN_EXISTS = -6,             // File exists (EEXIST)
    KERN_IO_ERROR = -7,           // I/O error (EIO)
    KERN_INTERRUPTED = -8,        // Interrupted system call (EINTR)
    KERN_NOT_SUPPORTED = -9,      // Operation not supported (ENOTSUP)
    KERN_WRONG_TYPE = -10,        // Incorrect type provided (custom)
    KERN_MAX_REACHED = -11,       // Maximum limit reached (custom)
    KERN_NO_SYS = -12,            // No system (custom)

    // Filesystem and Mounting Errors
    KERN_NO_MOUNT = -13,          // No mount point found (custom)
    KERN_ALREADY_MOUNTED = -14,   // Already mounted (custom)
    KERN_NOT_MOUNTED = -15,       // Not mounted (custom)
    KERN_FS_CORRUPTED = -16,      // Filesystem corrupted (custom)
    KERN_FS_READ_ONLY = -17,      // Filesystem is read-only (EROFS)
    KERN_NO_SPACE = -18,          // No space left on device (ENOSPC)
    KERN_QUOTA_EXCEEDED = -19,    // Quota exceeded (EDQUOT)
    KERN_BAD_HANDLE = -20,        // Bad handle (custom)

    // Process Management Errors
    KERN_NO_CHILD = -21,          // No child processes (ECHILD)
    KERN_ZOMBIE_PROCESS = -22,    // Process is in zombie state (custom)
    KERN_ACCESS_DENIED = -23,     // Access denied (EACCES)
    KERN_NO_RESOURCE = -24,       // No resources available (ENOMEM)

    // Networking Errors
    KERN_NET_DOWN = -25,          // Network is down (ENETDOWN)
    KERN_NET_UNREACH = -26,       // Network unreachable (ENETUNREACH)
    KERN_NET_RESET = -27,         // Connection reset by network (ENETRESET)
    KERN_CONN_ABORTED = -28,      // Connection aborted (ECONNABORTED)
    KERN_CONN_RESET = -29,        // Connection reset by peer (ECONNRESET)
    KERN_NO_BUFFER = -30,         // No buffer space available (ENOBUFS)
    KERN_CONN_REFUSED = -31,      // Connection refused (ECONNREFUSED)
    KERN_ADDR_IN_USE = -32,       // Address already in use (EADDRINUSE)
    KERN_ADDR_NOT_AVAIL = -33,    // Address not available (EADDRNOTAVAIL)

    // Device and Driver Errors
    KERN_NO_DEVICE = -34,         // No such device (ENODEV)
    KERN_DEVICE_NOT_READY = -35,  // Device not ready (custom)
    KERN_DEVICE_FAILED = -36,     // Device failure (custom)
    KERN_DRIVER_NOT_FOUND = -37,  // Driver not found (custom)
    KERN_DRIVER_FAILED = -38,     // Driver initialization failed (custom)

    // Synchronization and Locking Errors
    KERN_DEADLOCK = -39,          // Deadlock detected (EDEADLK)
    KERN_LOCK_FAILED = -40,       // Lock acquisition failed (custom)
    KERN_ALREADY_LOCKED = -41,    // Resource already locked (custom)

    // Memory and Resource Allocation Errors
    KERN_OUT_OF_BOUNDS = -42,     // Access out of bounds (custom)
    KERN_OVERFLOW = -43,          // Overflow detected (custom)
    KERN_UNDERFLOW = -44,         // Underflow detected (custom)
    KERN_NULL_POINTER = -45,      // Null pointer dereferenced (custom)
    KERN_PAGE_FAULT = -46,        // Page fault occurred (custom)

    // Miscellaneous Errors
    KERN_TIMEOUT = -47,           // Operation timed out (ETIMEDOUT)
    KERN_UNSUPPORTED_OPERATION = -48, // Operation unsupported in current state (custom)
    KERN_UNEXPECTED = -49,        // Unexpected error (custom)
    KERN_ILLEGAL_OPERATION = -50, // Illegal operation (custom)
    KERN_BAD_DESCRIPTOR = -51,    // Bad file descriptor (EBADF)
    KERN_OVERLOADED = -52,        // System overloaded (custom)
    KERN_TOO_BIG = -53,           // File or similar is larger than this function would like to deal with
};


static inline uint64_t min(uint64_t one, uint64_t two) {
    return one > two ? two : one;
}

static inline uint64_t max(uint64_t one, uint64_t two) {
    return one < two ? two : one;
}

static inline void to_big_endian(void *pointer, uint64_t bytes) {
    char *value = pointer;
    for (int i = 0; i < bytes; i += 2) {
        char temp = value[i];
        value[i] = value[i + 1];
        value[i + 1] = temp;
    }
}

/*
 * Core function prototypes
 */

int64_t write(uint64_t handle, char *buffer, uint64_t bytes);

int64_t read(uint64_t handle, char *buffer, uint64_t bytes);

int64_t open(char *path);

void close(uint64_t handle);

int64_t mount(char *mount_point, char *mounted_filesystem);

int64_t unmount(char *path);

int64_t rename(char *path, char *new_name);

int64_t create(char *path, char *name, uint64_t type);

int64_t get_size(uint64_t handle);

int64_t seek(uint64_t handle, uint64_t whence);

void ksprintf(char *buffer, char *str, ...);

void warn_printf(char *str, ...);

void info_printf(char *str, ...);

void kprintf(char *str, ...);

void kprintf_color(uint32_t color, char *str, ...);

void err_printf(char *str, ...);

void debug_printf(char *str, ...);

void serial_printf(char *str,...);

void log_kernel_message(const char *message);

#define DIONYSOS_ASCII_STRING \
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
"   '    '       '        '    '        '       '          '             '    '            '    '          '    '      \n"
#define DIONYSOS_ASCII_STRING2 "                                ___           ___                       ___           ___           ___     \n" \
"     _____                     /\\  \\         /\\  \\                     /\\__\\         /\\  \\         /\\__\\    \n" \
"    /::\\  \\       ___         /::\\  \\        \\:\\  \\         ___       /:/ _/_       /::\\  \\       /:/ _/_   \n" \
"   /:/\\:\\  \\     /\\__\\       /:/\\:\\  \\        \\:\\  \\       /|  |     /:/ /\\  \\     /:/\\:\\  \\     /:/ /\\  \\  \n" \
"  /:/  \\:\\__\\   /:/__/      /:/  \\:\\  \\   _____\\:\\  \\     |:|  |    /:/ /::\\  \\   /:/  \\:\\  \\   /:/ /::\\  \\ \n" \
" /:/__/ \\:|__| /::\\  \\     /:/__/ \\:\\__\\ /::::::::\\__\\    |:|  |   /:/_/:/\\:\\__\\ /:/__/ \\:\\__\\ /:/_/:/\\:\\__\\\n" \
" \\:\\  \\ /:/  / \\/\\:\\  \\__  \\:\\  \\ /:/  / \\:\\~~\\~~\\/__/  __|:|__|   \\:\\/:/ /:/  / \\:\\  \\ /:/  / \\:\\/:/ /:/  /\n" \
"  \\:\\  /:/  /   ~~\\:\\/\\__\\  \\:\\  /:/  /   \\:\\  \\       /::::\\  \\    \\::/ /:/  /   \\:\\  /:/  /   \\::/ /:/  / \n" \
"   \\:\\/:/  /       \\::/  /   \\:\\/:/  /     \\:\\  \\      ~~~~\\:\\  \\    \\/_/:/  /     \\:\\/:/  /     \\/_/:/  /  \n" \
"    \\::/  /        /:/  /     \\::/  /       \\:\\__\\          \\:\\__\\     /:/  /       \\::/  /        /:/  /   \n" \
"     \\/__/         \\/__/       \\/__/         \\/__/           \\/__/     \\/__/         \\/__/         \\/__/    \n"

#define DIONYSOS_ASCII_STRING3 "       _             _          _            _           _        _         _            _            _        \n" \
"      /\\ \\          /\\ \\       /\\ \\         /\\ \\     _  /\\ \\     /\\_\\      / /\\         /\\ \\         / /\\      \n" \
"     /  \\ \\____     \\ \\ \\     /  \\ \\       /  \\ \\   /\\_\\\\ \\ \\   / / /     / /  \\       /  \\ \\       / /  \\     \n" \
"    / /\\ \\_____\\    /\\ \\_\\   / /\\ \\ \\     / /\\ \\ \\_/ / / \\ \\ \\_/ / /     / / /\\ \\__   / /\\ \\ \\     / / /\\ \\__  \n" \
"   / / /\\/___  /   / /\\/_/  / / /\\ \\ \\   / / /\\ \\___/ /   \\ \\___/ /     / / /\\ \\___\\ / / /\\ \\ \\   / / /\\ \\___\\ \n" \
"  / / /   / / /   / / /    / / /  \\ \\_\\ / / /  \\/____/     \\ \\ \\_/      \\ \\ \\ \\/___// / /  \\ \\_\\  \\ \\ \\ \\/___/ \n" \
" / / /   / / /   / / /    / / /   / / // / /    / / /       \\ \\ \\        \\ \\ \\     / / /   / / /   \\ \\ \\       \n" \
"/ / /   / / /   / / /    / / /   / / // / /    / / /         \\ \\ \\   _    \\ \\ \\   / / /   / / /_    \\ \\ \\      \n" \
"\\ \\ \\__/ / /___/ / /__  / / /___/ / // / /    / / /           \\ \\ \\ /_/\\__/ / /  / / /___/ / //_\\__/ / /      \n" \
" \\ \\___\\/ //\\__\\/ /___\\/ / /____\\/ // / /    / / /             \\ \\_\\\\ \\/___/ /  / / /____\\/ / \\ \\/___/ /       \n" \
"  \\/_____\\/\\/_________/\\/_________/ \\/_/     \\/_/               \\/_/ \\_____\\/   \\/_________/   \\_____\\/        \n"

#define AUTHOR_ASCII_STRING " __        __    _ _   _               _             ____            _                  ____ _ _     _     \n \\ \\      / / __(_) |_| |_ ___ _ __   | |__  _   _  |  _ \\ _   _ ___| |_ _   _ _ __    / ___(_) |__ | |__  \n  \\ \\ /\\ / / '__| | __| __/ _ \\ '_ \\  | '_ \\| | | | | | | | | | / __| __| | | | '_ \\  | |  _| | '_ \\| '_ \\ \n   \\ V  V /| |  | | |_| ||  __/ | | | | |_) | |_| | | |_| | |_| \\__ \\ |_| |_| | | | | | |_| | | |_) | |_) |\n    \\_/\\_/ |_|  |_|\\__|\\__\\___|_| |_| |_.__/ \\__, | |____/ \\__,_|___/\\__|\\__, |_| |_|  \\____|_|_.__/|_.__/ \n                                             |___/                       |___/                              \n"

#define DIONYSOS_ART \
".    .\n" \
"    .       =O===\n" \
" . _       %- - %%%\n" \
"  (_)     % <    D%%%\n" \
"   |      ' ,  ,$$$\n" \
"  -/-     %%%%  |\n" \
"   |//; ,---' _ |----.\n" \
"    \\ )(           /  )\n" \
"    | \\/ \\.   '  _.|,  \\\n" \
"    |  \\ /(   /    / \\_ \\\n" \
"     \\ /  (        | /  )\n" \
"          (  ,  ___|/ ,`\n" \
"          (   _/ (  |  \\_\n" \
"        _/\\--//     ,\\\\\\\n" \
"  _______ _/\\\\   / |      |_________\n" \
"|_______/   \\\\ / _/   ,   |________|\n" \
"__|___#/     \\  _  '      |##____|___\n" \
"/______/ _    |  / \\   |   /__________\\\n" \
"  |   /           _/\\_/  b'ger\n" \
"  /_     '_,____\\/ )(\n" \
" |/ \\_   /        (  \\\n" \
"  | /-\\_/          Oooo\n" \
"  )(. \n" \
" /  )\n" \
"oooO\n"

#define ICARUS_ART \
"                 )\n" \
"               /  )\n" \
"              /  / )\n" \
"         -   /  / /\n" \
"            '  / / -\n" \
"           / _/ / /\n" \
"     _    / _/_, /          ,\n" \
"   + $$$ / _/_/_/          \\       |\n" \
" /- + $$/ _/_/_/      /\n" \
" \\`_ $$/'_/_/    .    ______   _\n" \
"    \\ (  / ___,_____ _ _____,\n" \
"    |  `(|/_,_,__ ________/\n" \
"    |.   |''_,_______)\n" \
"     \\   (_\n" \
"      \\  / |-._\n" \
"       \\.' /|/ \\_._\n" \
"       /_/   _/    /-'__\n" \
"         \\     \\'       \\.___\n" \
"          '.   /,     |_/_   |._\n" \
"            \\ / )   '.     '_/, )\n" \
"             (_(     -\\_   /  \\ \\\n" \
"                \\__      |-'   |/\n" \
"                  \\._  /_/_\n" \
"                     \\_/\\' )\n" \
"                         \\ |\n" \
"                         |/   b'ger\n" \
"\n" \
"              ~~~~~~~\n" \
"       ~~~~~~~~~   ~~~~\n"
#endif
