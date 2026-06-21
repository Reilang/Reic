/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * vector.h — Macro-based type-safe generic dynamic array.
 *
 * DECLARE_VECTOR(type, name) expands to a struct (name##_vector) and a set
 * of inline functions: new, push, pop, get, free.  The vector grows by
 * doubling when full.  All allocation failures are fatal (abort).
 *
 * Usage:
 *   DECLARE_VECTOR(int, int)   // produces int_vector, int_vec_new, int_vec_push, ...
 *   int_vector v;
 *   int_vec_new(&v, 16);       // initial capacity 16
 *   int_vec_push(&v, 42);
 *   int x = int_vec_get(&v, 0);
 *   int_vec_free(&v);
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 * Copyright (C) 2026  LLLichlet
 *
 * This file is part of ReiC.
 *
 * ReiC is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * ReiC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with ReiC.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef COLLECT_VECTOR_H
#define COLLECT_VECTOR_H

#include <stdlib.h>

/*
 * Generates a type-safe dynamic array for the given element type.
 *
 * Produced symbols (suffix):
 *   _vector   — struct { type *data; int size; int cap; }
 *   _new      — allocate with initial capacity (aborts on OOM)
 *   _push     — append element, doubling capacity as needed (aborts on OOM)
 *   _pop      — remove and return last element (aborts if empty)
 *   _get      — access element by index (aborts if out of bounds)
 *   _free     — release heap memory, zero out metadata
 */
#define DECLARE_VECTOR(type, name) \
    typedef struct { \
        type *data; \
        int size; \
        int cap; \
    } name##_vector; \
    static inline void name##_vec_new(name##_vector *v, int cap) \
    { \
        v->data = (type*)malloc((size_t)cap * sizeof(type)); \
        if (!v->data) { \
            abort(); \
        } \
        v->size = 0; \
        v->cap = cap; \
    } \
    static inline void name##_vec_push(name##_vector *v, type val) \
    { \
        type *tmp; \
        if (v->size >= v->cap) { \
            int new_cap = v->cap == 0 ? 1 : v->cap * 2; \
            tmp = (type*)realloc(v->data, (size_t)new_cap * sizeof(type)); \
            if (!tmp) { \
                abort(); \
            } \
            v->data = tmp; \
            v->cap = new_cap; \
        } \
        v->data[v->size++] = val; \
    } \
    static inline type name##_vec_pop(name##_vector *v) \
    { \
        if (v->size == 0) { \
            abort(); \
        } \
        return v->data[--v->size]; \
    } \
    static inline type name##_vec_get(const name##_vector *v, int idx) \
    { \
        if (idx < 0 || idx >= v->size) { \
            abort(); \
        } \
        return v->data[idx]; \
    } \
    static inline void name##_vec_free(name##_vector *v) \
    { \
        free(v->data); \
        v->data = NULL; \
        v->size = v->cap = 0; \
    }

#endif /* COLLECT_VECTOR_H */
