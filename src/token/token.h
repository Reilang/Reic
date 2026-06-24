/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * token.h — Lexical token type definitions and the token data structure.
 *
 * Defines the token type enum covering identifiers, literals, operators,
 * delimiters, and EOF.  Each token carries its source position (line/col)
 * and a union value appropriate to its type.
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

    TK_ADD,
    TK_MINUS,
    TK_STAR,
    TK_SLASH,
    TK_EQUAL,

    TK_OPAREN,
    TK_CPAREN,
    TK_OBRACKET,
    TK_CBRACKET,
    TK_OBRACE,
    TK_CBRACE,
    TK_COMMA,
    TK_OABRACKET,
    TK_CABRACKET,

    TK_COLON,
    TK_NOT,
    TK_DOT,

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
    int col;              /* 0-based column within the line */
} token;

DECLARE_VECTOR(token, token)

/*
 * Allocates and returns a human-readable debug string for a token.
 * Caller must free() the result.
 */
char *token_print(token token_);

/* Returns the display symbol for an operator token type (e.g. TK_ADD -> "+"). */
const char *binop_symbol(tktype op);

#endif /* TOKEN_TOKEN_H */
