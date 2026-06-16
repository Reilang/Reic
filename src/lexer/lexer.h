/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * lexer.h — Source representation and lexer state for the tokenizer.
 *
 * The src struct tracks raw input text and the current line/column cursor.
 * The lexer struct wraps src with a working buffer (readnow) used during
 * token construction.  The tokenize() entry point converts raw source into
 * a stream of tokens and diagnostics.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifndef LEXER_LEXER_H
#define LEXER_LEXER_H

#include "diag/diag.h"
#include "token/token.h"

typedef enum {
    LSTATE_NORMAL,
    LSTATE_IDENT,
    LSTATE_ILITER,
    LSTATE_FLITER,
    LSTATE_SLITER,
    LSTATE_CLITER,
    LSTATE_COMMENT,
    LSTATE_ERROR,
    LSTATE_END
} lstate;

typedef struct {
    char *raw;
    int line;
    int column;
} src;

typedef struct {
    lstate state;
    char readnow[256];
    src src_;
    int paren_depth;
} lexer;

void tokenize(lexer *lexer_, token_vector *tokens, diag_vector *diags);

#endif /* LEXER_LEXER_H */
