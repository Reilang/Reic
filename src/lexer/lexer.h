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

/* Lexer state machine states. */
typedef enum {
    LSTATE_NORMAL,    /* top-level dispatch */

    LSTATE_IDENT,     /* accumulating an identifier or keyword */
    LSTATE_ILITER,    /* accumulating an integer literal */
    LSTATE_FLITER,    /* accumulating a float literal (past the dot) */
    LSTATE_SLITER,    /* inside a string literal "..." */
    LSTATE_CLITER,    /* inside a character literal '...' */

    LSTATE_COMMENT,   /* line comment (; to end of line) */

    LSTATE_ERROR,     /* recoverable error — emit and reset */
    LSTATE_END        /* end of input reached */
} lstate;

/*
 * Source text handle.  .raw is advanced by the lexer during scanning;
 * .line and .column track the current position for diagnostics.
 */
typedef struct {
    char *raw;        /* pointer into the source buffer (mutated during scan) */
    int line;         /* 1-based */
    int column;       /* 0-based */
} src;

/*
 * Lexer instance.
 *
 *   state       — current state-machine state
 *   readnow     — working buffer for the token being built (max 255 chars)
 *   src_        — source text handle
 *   paren_depth — nesting depth of ( ) — when > 0, newlines are suppressed
 *                 so that multi-line expressions inside parens work naturally
 */
typedef struct {
    lstate state;
    char readnow[256];
    src src_;
    int paren_depth;
} lexer;

/*
 * Tokenizes the source text.  Appends tokens to *tokens and diagnostics
 * (errors, warnings) to *diags.  Stops when the source is exhausted or
 * an unrecoverable error occurs.
 */
void tokenize(lexer *lexer_, token_vector *tokens, diag_vector *diags);

#endif /* LEXER_LEXER_H */
