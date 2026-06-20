/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * parser.c — Entry point and shared helpers for the recursive-descent parser.
 *
 * parse() drives the top-level parse loop.  Helper functions (token cursor,
 * node allocation, sync, block-body parsing) are shared across the other
 * parser_*.c translation units via parser_internal.h.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "parser/parser.h"
#include "parser/parser_internal.h"
#include "type/type.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- token cursor helpers ---- */

token curtok(const parser *p)
{
    if (p->cursor >= p->tokens.size) {
        token eof = {0};
        eof.type = TK_EOF;
        return eof;
    }
    return token_vec_get(&p->tokens, p->cursor);
}

int at_eof(const parser *p)
{
    return p->cursor >= p->tokens.size
        || token_vec_get(&p->tokens, p->cursor).type == TK_EOF;
}

void skip_newlines(parser *p)
{
    while (p->cursor < p->tokens.size
           && token_vec_get(&p->tokens, p->cursor).type == TK_NEXTLINE)
        p->cursor++;
}

/* ---- node allocation helpers ---- */

int new_node(node_vector *nodes, anode_kind kind)
{
    anode n = {0};
    n.kind = kind;
    n.child = -1;
    n.next = -1;
    node_vec_push(nodes, n);
    return nodes->size - 1;
}

int new_ident(node_vector *nodes, const char *name, anode_kind kind)
{
    int idx = new_node(nodes, kind);
    nodes->data[idx].sv = strdup(name);
    return idx;
}

/*
 * Skip tokens until a statement boundary (newline, brace, EOF).
 * Used for error recovery so one syntax error doesn't trigger a cascade.
 */
void sync(parser *p)
{
    while (p->cursor < p->tokens.size) {
        tktype t = token_vec_get(&p->tokens, p->cursor).type;
        if (t == TK_NEXTLINE || t == TK_CBRACE || t == TK_OBRACE || t == TK_EOF)
            return;
        p->cursor++;
    }
}

/* ---- block body ---- */

/*
 * Parse statements inside a braced block.  The caller must consume the opening
 * brace and push PSTATE_BLOCK before calling this function.
 *
 * Returns the ANODE_BLOCK node index.  'context' names the enclosing construct
 * for use in EOF error messages.
 */
int parse_block_body(parser *p, node_vector *nodes, diag_vector *diags,
                     const char *context)
{
    int initial_sz = p->states.size;
    int block_idx = new_node(nodes, ANODE_BLOCK);
    int first_stmt = -1;
    int last_stmt = -1;

    while (p->states.size >= initial_sz && p->cursor < p->tokens.size) {
        skip_newlines(p);
        if (at_eof(p)) {
            char efbuf[128];
            snprintf(efbuf, sizeof(efbuf), "unexpected end of file in %s",
                     context);
            diag_add(diags, LEVEL_ERROR, efbuf,
                     curtok(p).line, curtok(p).column);
            break;
        }

        if (state_vec_get(&p->states, p->states.size - 1) == PSTATE_BLOCK) {
            token tk = curtok(p);
            if (tk.type == TK_CBRACE) {
                p->cursor++;
                state_vec_pop(&p->states);
                continue;
            }
            if (tk.type == TK_OBRACE) {
                p->cursor++;
                state_vec_push(&p->states, PSTATE_BLOCK);
                continue;
            }
            {
                int sidx = parse_stmt(p, nodes, diags);
                if (sidx >= 0) {
                    if (first_stmt < 0)
                        first_stmt = sidx;
                    else
                        nodes->data[last_stmt].next = sidx;
                    last_stmt = sidx;
                }
            }
        }
    }

    nodes->data[block_idx].child = first_stmt;
    return block_idx;
}

/* ---- main entry point ---- */

void parse(parser *p, node_vector *nodes, diag_vector *diags)
{
    while (p->cursor < p->tokens.size) {
        skip_newlines(p);
        if (at_eof(p))
            break;

        if (p->states.size == 0) {
            token tk = curtok(p);
            if (tk.type == TK_CBRACE) {
                diag_add(diags, LEVEL_ERROR, "unexpected '}'",
                        tk.line, tk.column);
                p->cursor++;
            } else if (tk.type == TK_OBRACE) {
                p->cursor++;
                state_vec_push(&p->states, PSTATE_BLOCK);
            } else {
                parse_stmt(p, nodes, diags);
            }
        } else {
            switch (state_vec_get(&p->states, p->states.size - 1)) {
            case PSTATE_BLOCK: {
                token tk = curtok(p);
                if (tk.type == TK_CBRACE) {
                    p->cursor++;
                    state_vec_pop(&p->states);
                } else if (tk.type == TK_OBRACE) {
                    p->cursor++;
                    state_vec_push(&p->states, PSTATE_BLOCK);
                } else {
                    parse_stmt(p, nodes, diags);
                }
                break;
            }
            default:
                break;
            }
        }
    }
}
