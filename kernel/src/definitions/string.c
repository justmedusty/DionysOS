//
// Created by dustyn on 9/19/24.
//

#include "include/definitions/string.h"
#include "include/definitions.h"
#include "include/arch/arch_cpu.h"
#include "include/drivers/serial/uart.h"

/*
    Need to ensure there is enough memory in your string
 */
void strcpy(char* dest, char* src) {
    while ((*dest++ = *src++));
}

/*
 * This function is a safe version of strcpy that will take a size and stick to it regardless of how much is left
 */
void safe_strcpy(char* dest, char* src, uint64_t dest_size) {
    uint64_t pointer = 0;
    while (((*dest++ = *src++)) && (pointer++ < dest_size));

    if(pointer == dest_size) {
        dest[dest_size - 1] = '\0';
    }
}

/*
 * Your run-of-the-mill concat string function, adds to the end of a string. This function is pretty dangerous so I may write a safe one later.
 */
void strcat(char* str1, char* str2) {
    while (*str1 != '\0') {
        str1++;
    }

    while (*str2 != '\0') {
        *str1++ = *str2++;
    }
}

/*
 * Your run-of-the-mill string compare c function
 */
uint64_t strcmp(char* str1, char* str2) {
    while (*str1 != '\0' && *str2 != '\0') {
        if (*str1 != *str2) {
            return 0;
        }
        str1++;
        str2++;
    }
    return 1;
}

uint64_t safe_strcmp(char* str1, char* str2,uint64_t max_len) {
    while (*str1 != '\0' && *str2 != '\0' && max_len > 0) {
        if (*str1 != *str2) {
            return 0;
        }
        str1++;
        str2++;
        max_len--;
    }
    return 1;
}


/*
 * Your run-of-the-mill string length function, walk and count until terminator (I'll be back)
 */
uint64_t strlen(const char* src) {
    uint64_t length = 0;
    while (src[length] != '\0') {
        length++;
    }
    return length;
}

/*
 *    Tokenizing the string using a single character as a delimiter
 *  This is quite different from the regular impl of strtok, this one is thread safe since it contains no internal references.
 *  This is designed around the main use being path parsing.
 */
uint64_t strtok(char* str, char delimiter, char* token, uint64_t token_number) {
    if (*str == delimiter) {
        str++;
    }

    uint64_t current_token = 1;
    int index = 0;
    while (*str != '\0') {
        if (current_token == token_number && *str == delimiter) {
            break;
        }


        if ((*str) == delimiter && token_number != current_token) {
            str++;
            current_token++;
        }

        if (token_number == current_token) {
            *token = *str;
            token++;
        }
        str++;
    }

    *token = '\0';

    if (*str == '\0') {
        return LAST_TOKEN;
    }
    else {
        return NEXT_TOKEN;
    }
}
/*
 * This is a sort of helper function that will help with path parsing. The reasoning is my specialized strtok function returns token n, but that isn't very helpful if you do
 * not know how many tokes are in the filesystem path that you are reading, so this function exists for that.
 */
uint64_t strtok_count(char* str, char delimiter) {
    uint64_t count = 0;
    uint64_t last_token = NEXT_TOKEN;
    while (last_token != LAST_TOKEN) {
        char temp_string[128];
        last_token = strtok(str, delimiter, temp_string, UINT64_MAX);
        count++;
    }
    return count;
}

