/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * parser.h — State-machine parser driven by a state stack.
 *
 * parse() consumes a token_vector and emits a node_vector (flat AST).
 * Diagnostics are written to the caller-supplied diag_vector.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifndef PARSER_PARSER_H
#define PARSER_PARSER_H

#include "ast/ast.h"
#include "collect/vector.h"
#include "diag/diag.h"
#include "token/token.h"

/* Parser state stack entry.  Each state corresponds to a syntactic context. */
typedef enum {
    PSTATE_FUNC,
    PSTATE_STMT,
    PSTATE_EXPR,
    PSTATE_BLOCK,    /* inside { } — tracks brace nesting */

    PSTATE_IF,
    PSTATE_WHILE,
    PSTATE_FOR,
    PSTATE_RETURN,

    PSTATE_ERROR,
} pstate;

DECLARE_VECTOR(pstate, state)

/*
 * Parser instance.
 *
 *   states  — state stack (tracks nested syntactic contexts)
 *   tokens  — input token stream (consumed read-only via cursor)
 *   cursor  — current position in the token stream
 */
typedef struct {
    state_vector states;
    token_vector tokens;
    int cursor;
} parser;

/*
 * Parses the token stream, appending AST nodes to *nodes and errors to *diags.
 * On return, diags may contain errors — the caller should check has_level()
 * before proceeding to codegen.
 */
void parse(parser *p, node_vector *nodes, diag_vector *diags);

#endif /* PARSER_PARSER_H */
