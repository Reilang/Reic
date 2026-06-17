/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
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

typedef enum { 
    PSTATE_FUNC,
    PSTATE_STMT,
    PSTATE_EXPR,
    PSTATE_BLOCK,

    PSTATE_IF,
    PSTATE_WHILE,
    PSTATE_FOR,
    PSTATE_RETURN,

    PSTATE_ERROR,
} pstate;

DECLARE_VECTOR(pstate, state)

typedef struct {
    state_vector states;
    token_vector tokens;
    int cursor;
} parser;

void parse(parser *p, node_vector *nodes, diag_vector *diags);

#endif /* PARSER_PARSER_H */
