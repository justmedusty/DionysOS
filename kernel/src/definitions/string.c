//
// Created by dustyn on 9/19/24.
//

#include "include/definitions/string.h"
#include "include/definitions.h"
#include "include/arch/arch_cpu.h"

/*
    Need to ensure there is enough memory in your string
 */
void strcpy(char *dest, char *src) {
    while ((*dest++ = *src++));
}

void strcat(char *str1, char *str2) {
    while (*str1 !='\0') {
        str1++;
    }

    while(*str2 != '\0'){
      *str1++ = *str2++;
      }
}

uint64 strlen(char *src) {
    uint64 length = 0;
    while (src[length] != '\0') {
        length++;
    }
    return length;
}

/*
 *    Tokenizing the string using a single character as a delimiter
 */
char *strtok(char *str, char delimiter){
    char *next_token;
    while(str && *str != delimiter && *str != '\0'){
        next_token = str;
        next_token++;
        str++;
    }
    return next_token;
}