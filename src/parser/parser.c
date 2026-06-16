/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * parser.c — State-machine parser driven by a state stack.
 *
 * Reads a flat token_vector via cursor, emits AST nodes into the caller-supplied
 * node_vector.  The state stack drives grammar recognition; each state
 * corresponds to a syntactic construct currently being parsed.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "parser/parser.h"

char BUILTIN_TYPES[64][64] = {"int32"};

void parse(parser *parser_, node_vector *nodes, diag_vector *diags)
{
    token cur = {0};
    anode and = {0};
    diag dg = {0};

    (void)parser_;
    (void)nodes;
    (void)diags;
}
