//
// Created by dustyn on 9/19/24.
//

#pragma once
#include "include/types.h"

// For strtok to identiy whether returned token is the final token or not
#define LAST_TOKEN 0
#define NEXT_TOKEN 1

uint64 strlen(char *src);
void strcat(char *str1, char *str2);
void strcpy(char *dest, char *src);
uint64 strtok(char *str, char delimiter, char *token,uint64 token_number);
uint64 strcmp(char *str1, char *str2);
uint64 strtok_count(char* str, char delimiter);