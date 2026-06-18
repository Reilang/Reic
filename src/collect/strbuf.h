/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * strbuf.h — Dynamic string buffer with printf-style append.
 *
 * Grows automatically.  Allocation failures abort.
 *
 * Typical usage:
 *   strbuf sb;
 *   strbuf_init(&sb, 256);
 *   strbuf_add(&sb, "hello ");
 *   strbuf_addf(&sb, "world %d", 42);
 *   char *result = strbuf_detach(&sb);   // caller owns result, must free()
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifndef COLLECT_STRBUF_H
#define COLLECT_STRBUF_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Dynamic string buffer.  .data is always null-terminated.  Access .data
 * directly for reading; use strbuf_add/strbuf_addf for writing.
 */
typedef struct {
    char *data;     /* null-terminated buffer */
    size_t len;     /* string length (excluding null terminator) */
    size_t cap;     /* allocated capacity */
} strbuf;

/* Allocates the buffer with the given initial capacity.  Aborts on OOM. */
static inline void strbuf_init(strbuf *sb, size_t initial)
{
    sb->data = malloc(initial);
    if (!sb->data) abort();
    sb->data[0] = '\0';
    sb->len = 0;
    sb->cap = initial;
}

/* Ensures the buffer has room for need additional bytes.  Aborts on OOM. */
static inline void strbuf_grow(strbuf *sb, size_t need)
{
    if (sb->len + need + 1 <= sb->cap)
        return;
    size_t new_cap = (sb->len + need + 1) * 2;
    char *new_data = realloc(sb->data, new_cap);
    if (!new_data) abort();
    sb->data = new_data;
    sb->cap = new_cap;
}

/* Appends a C string (including null terminator). */
static inline void strbuf_add(strbuf *sb, const char *s)
{
    size_t slen = strlen(s);
    strbuf_grow(sb, slen);
    memcpy(sb->data + sb->len, s, slen + 1);
    sb->len += slen;
}

/* Appends a printf-formatted string. */
static inline void strbuf_addf(strbuf *sb, const char *fmt, ...)
{
    va_list ap;
    int need;

    va_start(ap, fmt);
    need = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if (need < 0) return;
    strbuf_grow(sb, (size_t)need + 1);

    va_start(ap, fmt);
    vsnprintf(sb->data + sb->len, sb->cap - sb->len, fmt, ap);
    va_end(ap);

    sb->len += (size_t)need;
}

/* Frees the internal buffer and resets the strbuf to empty. */
static inline void strbuf_free(strbuf *sb)
{
    free(sb->data);
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

/*
 * Returns the buffer and transfers ownership.
 * The strbuf is reset to empty; the caller must free() the returned pointer.
 */
static inline char *strbuf_detach(strbuf *sb)
{
    char *data = sb->data;
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
    return data;
}

#endif /* COLLECT_STRBUF_H */
