//
// Created by dustyn on 9/11/24.
//

#include "include/data_structures/hash_table.h"
#include <include/arch/arch_cpu.h>
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
uint32_t full = 0;

/*
 * A simple xor shift hash function. Obviously a non cryptographic hash. Modulus to ensure
 * we get wrap-around.
 *
 * Multiply by large primes and xor to try and get the most diffusion, was having problems with the previous hash function
 * causing too many collisions. This seems to work better.
 */
uint64_t hash(uint64_t key, uint64_t modulus) {

    key = key ^ key >> 4;
    key *= 0xBF58476D1CE4E5B9;
    key ^= (key >> 31);
    key *= 0x94D049BB133111EB;
    key ^= (key >> 30);
    key *= 0x9E3779B97F4A7C15;
    key ^= (key >> 26);

    if (key > modulus) {
        key %= modulus;
    }

    return key;
}

/*
 * Initialize a hash table of size, size.
 *
 * If the static pool is not full, ie if this is phys_init calling,
 * allocate all of the static pool hash buckets.
 *Init all of the lists
 */
void hash_table_init(struct hash_table* table, uint64_t size) {

    if (table == NULL) {
        panic("hash_table_init: table is NULL");
    }

    table->size = size;

    if(!full) {

        for (uint64_t i = 0; i < size; i++) {
            table->table[i] = hash_bucket_static_pool[i];
            singly_linked_list_init(&table->table[i],0);
        }
        full = 1;
        return;
    }

    table = kalloc(sizeof(struct singly_linked_list) * table->size);
    for (uint64_t i = 0; i < size; i++) {
        singly_linked_list_init(&table->table[i],0);
    }



}
/*
 * Free the tables buckets and the entire table
 */
void hash_table_destroy(struct hash_table* table) {
    kfree(table->table);
    kfree(table);
}
/*
 * Hashes the passed value, and inserts into the list at the hash index
 */
void hash_table_insert(struct hash_table* table, uint64_t key, void* data) {
    uint64_t hash_key = hash(key, table->size);
    singly_linked_list_insert_head(&table->table[hash_key], data);
}
/*
 * Retrieve a hash bucket based on a key passed
 */
struct singly_linked_list* hash_table_retrieve(struct hash_table* table, uint64_t hash_key) {
    struct singly_linked_list* hash_bucket = &table->table[hash_key];
    return hash_bucket;
}
