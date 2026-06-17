/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * strbuf.h — Dynamic string buffer with printf-style append.
 *
 * Grows automatically.  Allocation failures abort.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifndef COLLECT_STRBUF_H
#define COLLECT_STRBUF_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} strbuf;

static inline void strbuf_init(strbuf *sb, size_t initial)
{
    sb->data = malloc(initial);
    if (!sb->data) abort();
    sb->data[0] = '\0';
    sb->len = 0;
    sb->cap = initial;
}

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

static inline void strbuf_add(strbuf *sb, const char *s)
{
    size_t slen = strlen(s);
    strbuf_grow(sb, slen);
    memcpy(sb->data + sb->len, s, slen + 1);
    sb->len += slen;
}

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

static inline void strbuf_free(strbuf *sb)
{
    free(sb->data);
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

/* Returns the buffer and transfers ownership. Caller must free() the result. */
static inline char *strbuf_detach(strbuf *sb)
{
    char *data = sb->data;
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
    return data;
}

#endif /* COLLECT_STRBUF_H */
