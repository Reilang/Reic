/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                                     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * hashset.h — Macro-based type-safe generic hash set with open addressing.
 *
 * DECLARE_SET(type, name) expands to a struct (name##_set) and a set of inline
 * functions: new, insert, find, free.  Linear probing, power-of-two capacity,
 * amortized O(1) lookup.  All allocation failures are fatal (abort).
 *
 * Usage:
 *   DECLARE_SET(sym_entry, sym)
 *   sym_set s;
 *   sym_set_new(&s, 16, sym_hash, sym_eq);
 *   sym_set_insert(&s, entry);
 *   sym_entry *found = sym_set_find(&s, key);
 *   sym_set_free(&s);
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifndef COLLECT_HASHSET_H
#define COLLECT_HASHSET_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * FNV-1a hash over a byte range — small, fast, adequate for symbol tables.
 */
static inline uint64_t hash_fnv1a(const void *data, size_t len)
{
    const unsigned char *p = (const unsigned char*)data;
    uint64_t h = 0xcbf29ce484222325ULL;
    size_t i;
    for (i = 0; i < len; i++) {
        h ^= (uint64_t)p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

/*
 * Convenience: FNV-1a over a null-terminated string.
 */
static inline uint64_t hash_str(const char *s)
{
    return hash_fnv1a(s, strlen(s));
}

/* Round up to the next power of two (for table capacity). */
static inline int next_pow2(int n)
{
    int p = 1;
    if (n <= 0) return 1;
    while ((unsigned)p < (unsigned)n)
        p <<= 1;
    return p;
}

/*
 * Generates a type-safe open-addressing hash set for the given element type.
 *
 * Produced symbols (suffix):
 *   _set    — struct { type *entries; bool *occupied; int cap; int size;
 *                      hash_fn; eq_fn; }
 *   _new    — allocate with initial capacity (aborts on OOM)
 *   _insert — store element, return pointer (aborts on OOM during rehash)
 *   _find   — find by key, return pointer or NULL
 *   _free   — release heap memory, zero out metadata
 *
 * The caller provides hash_fn and eq_fn at _new time:
 *   hash_fn — uint64_t hash_fn(type)     (e.g. FNV-1a over a string field)
 *   eq_fn   — bool eq_fn(type, type)     (e.g. strcmp wrapper)
 */
#define DECLARE_SET(type, name) \
    typedef struct { \
        type *entries; \
        bool *occupied; \
        int cap; \
        int size; \
        uint64_t (*hash_fn)(type); \
        bool (*eq_fn)(type, type); \
    } name##_set; \
    \
    static inline void name##_set_new(name##_set *s, int cap, \
                                      uint64_t (*hash_fn)(type), \
                                      bool (*eq_fn)(type, type)) \
    { \
        int real_cap = next_pow2(cap); \
        s->entries = (type*)malloc((size_t)real_cap * sizeof(type)); \
        if (!s->entries) { \
            abort(); \
        } \
        s->occupied = (bool*)malloc((size_t)real_cap * sizeof(bool)); \
        if (!s->occupied) { \
            abort(); \
        } \
        memset(s->occupied, 0, (size_t)real_cap * sizeof(bool)); \
        s->cap = real_cap; \
        s->size = 0; \
        s->hash_fn = hash_fn; \
        s->eq_fn = eq_fn; \
    } \
    \
    static inline type *name##_set_insert(name##_set *s, type val); \
    \
    static inline void name##_set_rehash(name##_set *s, int new_cap) \
    { \
        type *old_entries = s->entries; \
        bool *old_occupied = s->occupied; \
        int old_cap = s->cap; \
        int i; \
        s->entries = (type*)malloc((size_t)new_cap * sizeof(type)); \
        if (!s->entries) { \
            abort(); \
        } \
        s->occupied = (bool*)malloc((size_t)new_cap * sizeof(bool)); \
        if (!s->occupied) { \
            abort(); \
        } \
        memset(s->occupied, 0, (size_t)new_cap * sizeof(bool)); \
        s->cap = new_cap; \
        s->size = 0; \
        for (i = 0; i < old_cap; i++) { \
            if (old_occupied[i]) { \
                type *ignore = name##_set_insert(s, old_entries[i]); \
                (void)ignore; \
            } \
        } \
        free(old_entries); \
        free(old_occupied); \
    } \
    \
    /* Inserts val.  Returns pointer to the stored element (new or existing). */ \
    static inline type *name##_set_insert(name##_set *s, type val) \
    { \
        uint64_t h; \
        int idx; \
        if (s->size * 2 >= s->cap) \
            name##_set_rehash(s, s->cap * 2); \
        h = s->hash_fn(val); \
        idx = (int)(h & (uint64_t)(s->cap - 1)); \
        for (;;) { \
            if (!s->occupied[idx]) { \
                s->entries[idx] = val; \
                s->occupied[idx] = true; \
                s->size++; \
                return &s->entries[idx]; \
            } \
            if (s->eq_fn(s->entries[idx], val)) { \
                s->entries[idx] = val; \
                return &s->entries[idx]; \
            } \
            idx = (idx + 1) & (s->cap - 1); \
        } \
    } \
    \
    /* Returns pointer to the stored element, or NULL if not found. */ \
    static inline type *name##_set_find(name##_set *s, type key) \
    { \
        uint64_t h; \
        int idx; \
        if (s->cap == 0) return NULL; \
        h = s->hash_fn(key); \
        idx = (int)(h & (uint64_t)(s->cap - 1)); \
        for (;;) { \
            if (!s->occupied[idx]) \
                return NULL; \
            if (s->eq_fn(s->entries[idx], key)) \
                return &s->entries[idx]; \
            idx = (idx + 1) & (s->cap - 1); \
        } \
    } \
    \
    static inline void name##_set_free(name##_set *s) \
    { \
        free(s->entries); \
        s->entries = NULL; \
        free(s->occupied); \
        s->occupied = NULL; \
        s->cap = 0; \
        s->size = 0; \
    }

#endif /* COLLECT_HASHSET_H */
