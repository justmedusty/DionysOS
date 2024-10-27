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
void safe_strcpy(char* dest, char* src, uint64 dest_size) {
    uint64 pointer = 0;
    while (((*dest++ = *src++)) && (pointer++ < dest_size));
}

/*
 * Your run of the mill concat string function, adds to the end of a string. This function is pretty dangerous so I may write a safe one later.
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
uint64 strcmp(char* str1, char* str2) {
    while (*str1 != '\0' && *str2 != '\0') {
        if (*str1 != *str2) {
            return 0;
        }
        str1++;
        str2++;
    }
    return 1;
}

/*
 * Your run-of-the-mill string length function, walk and count until terminator (I'll be back)
 */
uint64 strlen(const char* src) {
    uint64 length = 0;
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
uint64 strtok(char* str, char delimiter, char* token, uint64 token_number) {
    if (*str == delimiter) {
        str++;
    }

    uint64 current_token = 1;
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
uint64 strtok_count(char* str, char delimiter) {
    uint64 count = 0;
    uint64 last_token = NEXT_TOKEN;
    while (last_token != LAST_TOKEN) {
        //probably should bt smaller but this is fine for now
        char temp_string[4096];
        last_token = strtok(str, delimiter, temp_string, UINT64_MAX);
        count++;
    }
    return count;
}

