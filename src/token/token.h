/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * token.h — Lexical token type definitions and the token data structure.
 *
 * Defines the token type enum covering identifiers, literals, operators,
 * delimiters, and EOF.  Each token carries its source position (line/column)
 * and a union value appropriate to its type.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifndef TOKEN_TOKEN_H
#define TOKEN_TOKEN_H

#include <stdint.h>

#include "collect/vector.h"

/* Every lexical token the lexer can produce. */
typedef enum {
    TK_IDENT,      /* user-defined name (variable, function, type) */
    TK_KEYWORD,    /* reserved word (fn, var, if, while, loop, return) */

    TK_ILITER,     /* integer literal: 42, 0xFF */
    TK_FLITER,     /* floating-point literal: 3.14 */
    TK_SLITER,     /* string literal: "hello" */
    TK_CLITER,     /* character literal: 'x' */

    TK_ADD,        /* + */
    TK_MINUS,      /* - */
    TK_STAR,       /* * */
    TK_SLASH,      /* / */
    TK_EQUAL,      /* = */

    TK_OPAREN,     /* ( */
    TK_CPAREN,     /* ) */
    TK_OBRACKET,   /* [ */
    TK_CBRACKET,   /* ] */
    TK_OBRACE,     /* { */
    TK_CBRACE,     /* } */
    TK_COMMA,      /* , */
    TK_OABRACKET,  /* < */
    TK_CABRACKET,  /* > */

    TK_COLON,      /* : */
    TK_NOT,        /* ! */

    TK_NEXTLINE,   /* logical newline (statement separator) */
    TK_EOF,        /* end of input */

    /* Synthetic multi-character operators for match-arm comparison.
     * These are never emitted by the lexer; the parser combines
     * primitive tokens into these codes for storage in MATCHARM.op. */
    TK_GREATEREQUAL,  /* >= */
    TK_LESSEQUAL,     /* <= */
    TK_NOTEQUAL       /* != */
} tktype;

/* A single token produced by the lexer.  .type selects the active union member. */
typedef struct {
    tktype type;
    union {
        char *string;     /* TK_IDENT, TK_KEYWORD, TK_SLITER */
        int64_t integer;  /* TK_ILITER */
        double float_;    /* TK_FLITER */
        char char_;       /* TK_CLITER */
    } value;
    int line;             /* 1-based source line */
    int column;           /* 0-based column within the line */
} token;

DECLARE_VECTOR(token, token)

/*
 * Allocates and returns a human-readable debug string for a token.
 * Caller must free() the result.
 */
char *token_print(token token_);

#endif /* TOKEN_TOKEN_H */
