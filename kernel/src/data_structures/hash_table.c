//
// Created by dustyn on 9/11/24.
//

#include "include/data_structures/hash_table.h"

#include <include/types.h>

/*
 *  The modulus should be the number of buckets you have, obviously
 */
uint64 hash(uint64 key,uint64 modulus) {
    uint64 hash = ((key << 8 ^ key) ^ (key << 15 ^ key)) % modulus;
    return hash;
}