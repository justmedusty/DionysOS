//
// Created by dustyn on 9/11/24.
//

#pragma once
#include "include/types.h"
#include "include/data_structures/singly_linked_list.h"

struct hash_table {
    struct singly_linked_list **table;
    uint64 size;
};


uint64 hash(uint64 key,uint64 modulus);
void hash_table_init(struct hash_table* table, uint64 size);
void hash_table_destroy(struct hash_table *table);
void hash_table_insert(struct hash_table* table, uint64 key,void *data);
struct singly_linked_list *hash_table_retrieve(struct hash_table* table, uint64 hash_key);
