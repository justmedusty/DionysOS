//
// Created by dustyn on 9/19/24.
//

#include "include/definitions/string.h"

void strcpy(char *dest, char *src){

}

void strcat(char *str1, char *st2){

}

uint64 strlen(char *src){

  uint64 length = 0;
  while(src[length] != '\0'){
    length++;
    }
    return length;

}
/*
 *    Doing this my own way, just going to advcance the original pointer and return a copy of just the token
*     Will just support a 1 char delimter for now
 */

char *strtok(char *str, char *delimiter){
  char *next_token;
  uint32 index = 0;
  while(*str != *delimiter && *str != '\0'){
    *next_token++ = str[index++];
}


}