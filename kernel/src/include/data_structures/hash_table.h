//
// Created by dustyn on 9/11/24.
//

#pragma once
#include "include/types.h"
#include "include/data_structures/singly_linked_list.h"
#define HASH_TABLE_STATIC_POOL_SIZE 900
struct hash_table {
    struct singly_linked_list table[300];
    uint64_t size;
};



uint64_t hash(uint64_t key,uint64_t modulus);
void hash_table_init(struct hash_table* table, uint64_t size);
void hash_table_destroy(struct hash_table *table);
void hash_table_insert(struct hash_table* table, uint64_t key,void *data);
struct singly_linked_list *hash_table_retrieve(struct hash_table* table, uint64_t hash_key);
