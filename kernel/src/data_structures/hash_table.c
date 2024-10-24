//
// Created by dustyn on 9/11/24.
//

#include "include/data_structures/hash_table.h"
#include <include/types.h>
#include <include/arch/arch_cpu.h>
#include <include/drivers/uart.h>
#include <include/mem/kalloc.h>
#include "include/definitions.h"

/*
 * I may add support for growing the size of a hash table but for now I think I will just leave it
 */


/*
 *  The modulus should be the number of buckets you have, obviously
 *
 *  I will probably change this as time goes on and I get an idea how many collisions this causes
 */

struct singly_linked_list hash_bucket_static_pool[300];
uint32 full = 0;

uint64 hash(uint64 key, uint64 modulus) {
    uint64 hash = key ^ ((key << 8 ^ key) ^ (key << 3 ^ key));

    if (hash > modulus) {
        hash %= modulus;
    }

    return hash;
}


void hash_table_init(struct hash_table* table, uint64 size) {
    if (table == NULL) {
        panic("hash_table_init: table is NULL");
    }

    table->size = size;
    for (uint64 i = 0; i < size; i++) {
        table->table[i] = hash_bucket_static_pool[i];
        singly_linked_list_init(&table->table[i],0);
    }
}

void hash_table_destroy(struct hash_table* table) {
    kfree(table->table);
    kfree(table);
}

void hash_table_insert(struct hash_table* table, uint64 key, void* data) {
    uint64 hash_key = hash(key, table->size);
    singly_linked_list_insert_tail(&table->table[hash_key], data);

}
//
struct singly_linked_list* hash_table_retrieve(struct hash_table* table, uint64 hash_key) {
    struct singly_linked_list* hash_bucket = &table->table[hash_key];
    return hash_bucket;
}
