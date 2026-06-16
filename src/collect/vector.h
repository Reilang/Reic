/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * vector.h — Macro-based type-safe generic dynamic array.
 *
 * DECLARE_VECTOR(type, name) expands to a struct (name##_vector) and a set
 * of inline functions: new, push, pop, get, free.  The vector grows by
 * doubling when full.  All allocation failures are fatal (abort).
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifndef COLLECT_VECTOR_H
#define COLLECT_VECTOR_H

#include <stdlib.h>

#define DECLARE_VECTOR(type, name) \
    typedef struct { \
        type *data; \
        int size; \
        int cap; \
    } name##_vector; \
    static inline void name##_new(name##_vector *v, int cap) \
    { \
        v->data = (type*)malloc((size_t)cap * sizeof(type)); \
        if (!v->data) { \
            abort(); \
        } \
        v->size = 0; \
        v->cap = cap; \
    } \
    static inline void name##_push(name##_vector *v, type val) \
    { \
        type *tmp; \
        if (v->size >= v->cap) { \
            int ncap = v->cap == 0 ? 1 : v->cap * 2; \
            tmp = (type*)realloc(v->data, (size_t)ncap * sizeof(type)); \
            if (!tmp) { \
                abort(); \
            } \
            v->data = tmp; \
            v->cap = ncap; \
        } \
        v->data[v->size++] = val; \
    } \
    static inline type name##_pop(name##_vector *v) \
    { \
        if (v->size == 0) { \
            abort(); \
        } \
        return v->data[--v->size]; \
    } \
    static inline type name##_get(const name##_vector *v, int idx) \
    { \
        if (idx < 0 || idx >= v->size) { \
            abort(); \
        } \
        return v->data[idx]; \
    } \
    static inline void name##_free(name##_vector *v) \
    { \
        free(v->data); \
        v->data = NULL; \
        v->size = v->cap = 0; \
    }

#endif /* COLLECT_VECTOR_H */
