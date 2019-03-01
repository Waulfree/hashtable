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

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#define cast(type, val) (type)(val)

typedef intmax_t arch_st;
typedef size_t arch_t;

//Warning: requires gcc.
static inline arch_t arch_ilog2(arch_t n)
{
    return 8 * sizeof(arch_t) -
    #if SIZE_MAX <= 0xFFFFFFFF
        __builtin_clz(n)
    #elif SIZE_MAX <= 0xFFFFFFFFFFFFFFFF
        __builtin_clzl(n)
    #else
        __builtin_clzll(n)
    #endif
    - 1;
}

static inline arch_st arch_strcmp(const char* a, const char* b)
{
    return strcmp(a, b);
}

static inline arch_t arch_rand()
{
    return rand();
}

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

//djb2 hash function by Dan Bernstein.
static inline arch_t arch_strhash(const char* str)
{
    arch_t h = 5381;
    while (*str) h = ((h << 5) + h) ^ *str++;
    return h;
}

struct hash_table {
    struct pair* pairs;
    arch_t  size, totalsize,
        maxpairs, maxprobes,
        numpairs, hashseed;
};

void hash_table_init(struct hash_table* ht, arch_t size, arch_t maxpairs)
{
    arch_t maxprobes = arch_ilog2(size),
           totalsize = size + maxprobes;
    
    ht->pairs = cast(
        struct pair*,
        malloc(totalsize * sizeof(struct pair)));
    ht->size = size, ht->totalsize = totalsize,
    ht->maxpairs = maxpairs, ht->maxprobes = maxprobes,
    ht->numpairs = 0, ht->hashseed = arch_rand();
    
    for (arch_t i = 0; i < totalsize; i++){
         ht->pairs[i].status = PAIR_FREE;}
}

static inline void hash_table_clean(struct hash_table* ht)
{
    free(ht->pairs);
    //ht->pairs = NULL;
}

//Insert callbacks down there to support more hashing methods, or try overloading
void hash_table_resize(struct hash_table* ht);

static inline sstruct pair* hash_table_search(struct hash_table* ht, const void* key)
{
    arch_t idx = (arch_strhash(key) ^ ht->hashseed) % ht->size;
    struct pair* p = &ht->pairs[idx];
    
    for (arch_t dist = 0; dist < ht->maxprobes; dist++, p++)
        if (p->dist == dist) 
            if (!arch_strcmp(p->key, key)) return p;
    return NULL;
}

struct pair* hash_table_insert(struct hash_table* ht, struct pair cur)
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
        } 
        if (cur.dist >= ht->maxprobes) {
            hash_table_resize(ht);
            cur.dist = DIST_IDEAL;
            goto begin;
        } else if (p->dist == cur.dist) //check if key already exist
            if (!arch_strcmp(p->key, cur.key)) return NULL;
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
    struct hash_table new;
    hash_table_init(&new, ht->size * 2, ht->maxpairs * 2);
    for (struct pair* end = p + ht->totalsize; p < end; p++)
        if (p->status != PAIR_FREE) {
            p->dist = DIST_IDEAL;
            hash_table_insert(&new, *p);
        }
    hash_table_clean(ht);
    *ht = new;  
}

static inline struct hash_table* new_hash_table(arch_t size, arch_t maxpairs)
{
    struct hash_table* ht = cast(
        struct hash_table*,
        malloc(sizeof(struct hash_table)));
    hash_table_init(ht, size, maxpairs);
    return ht;
}

static inline void free_hash_table(struct hash_table* ht)
{
    hash_table_clean(ht);
    free(ht);//, ht = NULL;
}

#endif
