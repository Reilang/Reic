#ifndef PARSER_PARSER_H
#define PARSER_PARSER_H

#include "collect/vector.h"
#include "diag/diag.h"
#include "lexer/lexer.h"
#include "token/token.h"

typedef enum {
    PSTATE_FUNC,
    PSTATE_STMT,
    PSTATE_EXPR,
    PSTATE_IF,
    PSTATE_WHILE
} pstate;

DECLARE_VECTOR(pstate, state)

typedef struct {
    state_vector states;
    token_vector tokens;
    token readnow[64];
} parser;

void parse(parser *parser_, something *grammar, diag_vector *diags);

#endif /* PARSER_PARSER_H */
