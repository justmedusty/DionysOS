//
// Created by dustyn on 9/19/24.
//
#ifndef _STRING_H_
#define _STRING_H_
#pragma once
#include "include/types.h"
#include "include/memory/mem.h"

// For strtok to identiy whether returned token is the final token or not
#define LAST_TOKEN 0
#define NEXT_TOKEN 1

uint64_t strlen(const char *src);
void strcat(char *str1, char *str2);
void strcpy(char *dest, char *src);
uint64_t strtok(char *str, char delimiter, char *token,uint64_t token_number);
uint64_t strcmp(char *str1, char *str2);
uint64_t safe_strcmp(char* str1, char* str2,uint64_t max_len);
uint64_t strtok_count(char* str, char delimiter);
void safe_strcpy(char *dest, char *src,uint64_t dest_size);
#endif