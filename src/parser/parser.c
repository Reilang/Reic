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

void parse(parser *parser_, node_vector *nodes, diag_vector *diags)
{
    (void)parser_;
    (void)nodes;
    (void)diags;
}
