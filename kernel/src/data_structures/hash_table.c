//
// Created by dustyn on 9/11/24.
//

#include "include/data_structures/hash_table.h"
#include <include/types.h>
#include <include/mem/kalloc.h>

/*
 *  The modulus should be the number of buckets you have, obviously
 *
 *  I will probably change this as time goes on and I get an idea how many collisions this causes
 */
uint64 hash(uint64 key, uint64 modulus) {
    uint64 hash = ((key << 8 ^ key) ^ (key << 15 ^ key)) % modulus;
    return hash;
}


void hash_table_init(struct hash_table* table, uint64 size) {
    table->size = size;
    table->table = kalloc(size * sizeof(struct singly_linked_list*));

    for (uint64 i = 0; i < size; i++) {
        singly_linked_list_init(table->table[i]);
    }
}


void hash_table_destroy(struct hash_table* table) {
    kfree(table->table);
    kfree(table);
}

void hash_table_insert(struct hash_table* table, uint64 key,void *data) {
    uint64 hash_key = hash(key, table->size);
    singly_linked_list_insert_tail(table->table[hash_key], data);
}
