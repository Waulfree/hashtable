#ifndef __arch_guard__
#define __arch_guard__

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#define cast(type, val) (type)(val)
#define false 0
#define true 1

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

//djb2 hash function by Dan Bernstein.
static inline arch_t arch_strhash(const char* str)
{
    arch_t h = 5381;
    while (*str) h = ((h << 5) + h) ^ *str++;
    return h;
}

#endif