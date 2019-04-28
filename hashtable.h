/*
    Minimal hash table implementation
    designed by Maxim Kasperski.
    
    Features:
        - Open adressing
        - Linear probing combined with Robin Hood hashing;
        - Excludes unnecessary referencing, no border checks.
        
    This code has no copyrights.
    So, you're free to use it any way you want
    (steal, burn, sell or patent (nobody cares)).
*/

#ifndef __hash_table_guard__
#define __hash_table_guard__

#include "arch.h"

enum pair_status {
    PAIR_RESERVED = cast(arch_t, -3),
    PAIR_REMOVED,
    PAIR_FREE
};

enum pair_dist {
    DIST_IDEAL = cast(arch_t, 0)
};

struct pair {
    const void *key, *val;
    union {
        arch_t status, dist;
    };
};

static inline struct pair pair_init(const void* key, const void* val)
{
    //C-style cast warning!
    return (struct pair){key, val, {0}};
}

struct hash_table {
    struct pair* pairs;
    arch_t  size, totalsize,
        maxpairs, maxprobes,
        numpairs, hashseed;
};

static inline struct hash_table hash_table_init(arch_t size, arch_t maxpairs)
{
    struct hash_table ht;
    arch_t maxprobes = arch_ilog2(size),
           totalsize = size + maxprobes;
    
    ht.pairs = cast(
        struct pair*,
        malloc(totalsize * sizeof(struct pair)));
    ht.size = size, ht.totalsize = totalsize,
    ht.maxpairs = maxpairs, ht.maxprobes = maxprobes,
    ht.numpairs = 0, ht.hashseed = arch_rand();
    
    for (arch_t i = 0; i < totalsize; i++){
         ht.pairs[i].status = PAIR_FREE;}
    
    return ht;
}

static inline void hash_table_clean(struct hash_table* ht)
{
    free(ht->pairs);
    //ht->pairs = NULL;
}

//Insert callbacks down there to support more hashing methods, or try overloading
static inline void hash_table_resize(struct hash_table* ht);

struct pair* hash_table_search(struct hash_table* ht, const void* key)
{
    arch_t idx = (arch_strhash(key) ^ ht->hashseed) % ht->size;
    struct pair* p = &ht->pairs[idx];
    
    for (arch_t dist = 0; dist < ht->maxprobes; dist++, p++)
        if (p->dist == dist) 
            if (!arch_strcmp(p->key, key)) return p;
    return NULL;
}

static inline struct pair* hash_table_insert(struct hash_table* ht, struct pair cur)
{
    if (ht->numpairs >= ht->maxpairs) hash_table_resize(ht);
    begin:;
    arch_t idx = (arch_strhash(cur.key) ^ ht->hashseed) % ht->size;
    struct pair* p = &ht->pairs[idx];
    
    for (;; p++, cur.dist++) {
        if (p->status == PAIR_FREE) {
              *p = cur, ht->numpairs++;
              return p;
        } else if (p->dist > cur.dist) {
            struct pair buf = *p;
            *p = cur, cur = buf; //robin hood swap
        } if (cur.dist >= ht->maxprobes) {
            hash_table_resize(ht);
            cur.dist = DIST_IDEAL;
            goto begin;
        } else if (p->dist == cur.dist) //check if key already exist
            if (!arch_strcmp(p->key, cur.key)) {p->val = cur.val; return p;}
    }
}

static inline struct pair* hash_table_remove(struct hash_table* ht, const void* key)
{
    struct pair* p = hash_table_search(ht, key);
    if (p) p->status = PAIR_FREE;
    return p;
}

static inline void hash_table_resize(struct hash_table* ht)
{
    struct pair* p = ht->pairs;
    struct hash_table newht = hash_table_init(ht->size * 2, ht->maxpairs * 2);
    for (struct pair* end = p + ht->totalsize; p < end; p++)
        if (p->status != PAIR_FREE) {
            p->dist = DIST_IDEAL;
            hash_table_insert(&newht, *p);
        }
    hash_table_clean(ht);
    *ht = newht;  
}

static inline struct hash_table* new_hash_table(arch_t size, arch_t maxpairs)
{
    struct hash_table* ht = cast(
        struct hash_table*,
        malloc(sizeof(struct hash_table)));
    *ht = hash_table_init(size, maxpairs);
    return ht;
}

static inline void free_hash_table(struct hash_table* ht)
{
    hash_table_clean(ht);
    free(ht);//, ht = NULL;
}

#endif