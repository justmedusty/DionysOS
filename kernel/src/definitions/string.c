//
// Created by dustyn on 9/19/24.
//

#include "include/definitions/string.h"
#include "include/definitions.h"
#include "include/arch/arch_cpu.h"
#include "include/drivers/uart.h"

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

uint64 strcmp(char *str1, char *str2) {
  while (*str1 != '\0' && *str2 != '\0') {
    if (*str1 != *str2) {
         return 0;
    }
    str1++;
    str2++;
  }
  return 1;
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
*
 *  This is quite different from the regular impl of strtok, this one is threadsafe since it contains no internal references.
*   This is designed around the main use being path parsing.
 */
uint64 strtok(char *str, char delimiter, char *token,uint64 token_number) {

  if(*str == delimiter){
    str++;
  }

  uint64 current_token = 1;
  int index =0;
  while(*str != '\0' ){

    if(current_token == token_number && *str == delimiter){
        break;
    }


    if((*str) == delimiter &&  token_number != current_token){
         str++;
         current_token++;
    }

    if(token_number == current_token){
       *token = *str;
       token++;
     }
     str++;
    }

    *token = '\0';

    if(*str == '\0'){
      return LAST_TOKEN;
    }
    else{
      return NEXT_TOKEN;
    }
}