/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
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

typedef enum {
    TK_IDENT,
    TK_KEYWORD,

    TK_ILITER,
    TK_FLITER,
    TK_SLITER,
    TK_CLITER,

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
    TK_OABRACKET, /* < */
    TK_CABRACKET, /* > */

    TK_COLON,

    TK_NEXTLINE,
    TK_EOF
} tktype;

typedef struct {
    tktype type;
    union {
        char *string;
        int64_t integer;
        double float_;
        char char_;
    } value;
    int line;
    int column;
} token;

DECLARE_VECTOR(token, token)

char *token_print(token token_);

#endif /* TOKEN_TOKEN_H */
