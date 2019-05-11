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

enum hash_pair_status {PAIR_FREE = cast(arch_t, -2), PAIR_REMOVED};
enum hash_pair_dist {DIST_IDEAL = cast(arch_t, 0)};

//Be free to change types :)
typedef const char* pair_key_t;
typedef const void* pair_val_t;

struct pair {
    pair_key_t key;
    pair_val_t val;
};

struct hash_pair {
    union {
        struct pair pair;
        struct {
	    pair_key_t key;
	    pair_val_t val;
	};
    };
    union {
        arch_t status, dist;
    };
};

struct hash_table {
    struct hash_pair* pairs;
    arch_t  size, totalsize,
        maxpairs, maxprobes,
        numpairs, hashseed;
};

static inline struct pair pair_init(pair_key_t key, pair_val_t val)
{
    return (struct pair){key, val}; //C-style cast warning!
}

static inline struct hash_pair hash_pair_init(pair_key_t key, pair_val_t val)
{
    return (struct hash_pair){{{key, val}}, {0}}; //C-style cast warning!
}

static inline void hash_table_pairs_alloc(struct hash_table* ht, arch_t size)
{
    ht->pairs = cast(struct hash_pair*, malloc(size * sizeof(struct hash_pair)));
}

static inline void hash_table_pairs_free(struct hash_table* ht)
{
    free(ht->pairs);
}

static inline arch_t hash_table_key_index(struct hash_table* ht, pair_key_t key)
{
    return (arch_strhash(key) + ht->hashseed) % ht->size;
}

static inline bool hash_key_cmp(pair_key_t a, pair_key_t b)
{
    return !arch_strcmp(a, b);
}

static inline struct hash_table hash_table_init(arch_t size, arch_t maxpairs)
{
    struct hash_table ht;
    arch_t maxprobes = arch_ilog2(size),
           totalsize = size + maxprobes;

    hash_table_pairs_alloc(&ht, totalsize);
    ht.size = size, ht.totalsize = totalsize,
    ht.maxpairs = maxpairs, ht.maxprobes = maxprobes,
    ht.numpairs = 0, ht.hashseed = arch_rand();
    
    for (arch_t i = 0; i < totalsize; i++) 
	ht.pairs[i].status = PAIR_FREE;;
    
    return ht;	
}

struct pair* hash_table_search(struct hash_table* ht, pair_key_t key)
{
    arch_t idx = hash_table_key_index(ht, key);
    struct hash_pair* p = &ht->pairs[idx];
    
    for (arch_t dist = 0; dist < ht->maxprobes && p->status != PAIR_FREE; dist++, p++)
        if ((p->dist == dist) && hash_key_cmp(p->key, key)) return cast(struct pair*, p);
        
    return NULL;
}

static inline void hash_table_resize(struct hash_table* );
static inline struct pair* hash_table_insert(struct hash_table* ht, struct pair cur)
{
    while (true) {
        arch_t idx = hash_table_key_index(ht, cur.key);
        struct hash_pair* p = &ht->pairs[idx];
    
        for (arch_t dist = DIST_IDEAL; dist < ht->maxprobes; dist++, p++) {
            if (p->status >= PAIR_FREE) {
        	p->pair = cur, p->dist = dist, ht->numpairs++;
                return cast(struct pair*, p);
    	    } else if (p->dist > dist) {
                struct hash_pair buf = *p; //Robin Hood swap
                p->pair = cur, p->dist = dist,
                cur = buf.pair, dist = buf.dist;
            } else if (p->dist == dist && hash_key_cmp(p->key, cur.key)) {
        	p->val = cur.val;
        	return cast(struct pair*, p);
	    }
        }
        
        hash_table_resize(ht);
    }
}

static inline struct pair* hash_table_remove(struct hash_table* ht, pair_key_t key)
{
    struct hash_pair* p = cast(struct hash_pair*, hash_table_search(ht, key));
    if (p) p->status = PAIR_REMOVED, ht->numpairs--;
    return cast(struct pair*, p);
}

static inline void hash_table_resize(struct hash_table* ht)
{
    struct hash_pair* p = ht->pairs;
    struct hash_table newht = hash_table_init(ht->size * 2, ht->maxpairs * 2);
    for (struct hash_pair* end = p + ht->totalsize; p < end; p++)
        if (p->status < PAIR_FREE) hash_table_insert(&newht, *cast(struct pair*, p));
    hash_table_pairs_free(ht);
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
    hash_table_pairs_free(ht);
    free(ht);
}

#endif