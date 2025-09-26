//
// Created by dustyn on 9/11/24.
//

#pragma once

#include "include/definitions/types.h"
#include "include/data_structures/singly_linked_list.h"

#define HASH_TABLE_STATIC_POOL_SIZE 1500

struct static_hash_table {
    struct singly_linked_list table[1500];
    uint64_t size;
};

struct hash_table {
    struct singly_linked_list *table;
    uint64_t size;
};


uint64_t hash(uint64_t key, uint64_t modulus);

void static_hash_table_init(struct static_hash_table *table, uint64_t size);

void static_hash_table_destroy(struct static_hash_table *table);

void static_hash_table_insert(struct static_hash_table *table, uint64_t key, void *data);

struct singly_linked_list *static_hash_table_retrieve(struct static_hash_table *table, uint64_t hash_key);

void hash_table_init(struct hash_table *table, uint64_t size);

void hash_table_destroy(struct hash_table *table);

void hash_table_insert(struct hash_table *table, uint64_t key, void *data);

struct singly_linked_list *hash_table_retrieve(struct hash_table *table, uint64_t hash_key);

bool static_hash_table_check(struct static_hash_table* table, uint64_t hash_key,void *data);