/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * parser.c — State-machine parser driven by a state stack.
 *
 * Reads a flat token_vector via cursor, emits AST nodes into the caller-supplied
 * node_vector.  The state stack drives grammar recognition; each state
 * corresponds to a syntactic construct currently being parsed.
 *
 * Empty state stack = file level.  PSTATE_BLOCK tracks brace nesting so '}'
 * is handled at the right scope.  All other constructs are parsed synchronously
 * by dedicated functions (parse_funcdef, parse_return, etc.).
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "parser/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char BUILTIN_TYPES[64][64] = {"void", "int32"};

static int is_builtin_type(const char *name)
{
    int i;
    for (i = 0; i < 64; i++) {
        if (BUILTIN_TYPES[i][0] == '\0')
            return 0;
        if (strcmp(name, BUILTIN_TYPES[i]) == 0)
            return 1;
    }
    return 0;
}

static token curtok(const parser *p)
{
    if (p->cursor >= p->tokens.size) {
        token eof = {0};
        eof.type = TK_EOF;
        return eof;
    }
    return token_get(&p->tokens, p->cursor);
}

static int at_eof(const parser *p)
{
    return p->cursor >= p->tokens.size
        || token_get(&p->tokens, p->cursor).type == TK_EOF;
}

static void skip_newlines(parser *p)
{
    while (p->cursor < p->tokens.size
           && token_get(&p->tokens, p->cursor).type == TK_NEXTLINE)
        p->cursor++;
}

static int new_node(node_vector *nodes, anode_kind kind)
{
    anode n = {0};
    n.kind = kind;
    n.child = -1;
    n.next = -1;
    node_push(nodes, n);
    return nodes->size - 1;
}

static int new_ident(node_vector *nodes, const char *name, anode_kind kind)
{
    int idx = new_node(nodes, kind);
    nodes->data[idx].sv = strdup(name);
    return idx;
}

static int parse_stmt(parser *p, node_vector *nodes, diag_vector *diags);
static int parse_funcdef(parser *p, node_vector *nodes, diag_vector *diags);
static int parse_return(parser *p, node_vector *nodes, diag_vector *diags);

/*
 * We need to sync it. You don't want tons of errors, do you?
 */
static void sync(parser *p)
{
    while (p->cursor < p->tokens.size) {
        tktype t = token_get(&p->tokens, p->cursor).type;
        if (t == TK_NEXTLINE || t == TK_CBRACE || t == TK_OBRACE || t == TK_EOF)
            return;
        p->cursor++;
    }
}

static int parse_stmt(parser *p, node_vector *nodes, diag_vector *diags)
{
    token tk = curtok(p);

    if (tk.type == TK_KEYWORD) {
        if (strcmp(tk.value.string, "fn") == 0)
            return parse_funcdef(p, nodes, diags);
        if (strcmp(tk.value.string, "return") == 0)
            return parse_return(p, nodes, diags);
    }

    diag_add(diags, LEVEL_ERROR, "expected statement",
            tk.line, tk.column);
    sync(p);
    return -1;
}

static int parse_funcdef(parser *p, node_vector *nodes, diag_vector *diags)
{
    token tk;
    int name_idx, rettype_idx, func_idx;
    int param_head = -1;
    int param_tail = -1;
    int prev_link;
    int initial_sz;

    p->cursor++; /* skip 'fn' */

    skip_newlines(p);
    tk = curtok(p);
    if (tk.type != TK_IDENT) {
        diag_add(diags, LEVEL_ERROR, "expected function name after 'fn'",
                tk.line, tk.column);
        sync(p);
        return -1;
    }
    p->tokens.data[p->cursor].type = TK_IDENT_FUNC;
    name_idx = new_ident(nodes, tk.value.string, ANODE_IDENT_FUNC);
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_OPAREN) {
        diag_add(diags, LEVEL_ERROR, "expected '(' after function name",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;

    skip_newlines(p);
    while (curtok(p).type != TK_CPAREN) {
        int pname_idx, ptype_idx, vardecl_idx;

        if (at_eof(p)) {
            diag_add(diags, LEVEL_ERROR, "expected ')' to close parameter list",
                     tk.line, tk.column);
            sync(p);
            return -1;
        }

        skip_newlines(p);
        tk = curtok(p);
        if (tk.type != TK_IDENT) {
            diag_add(diags, LEVEL_ERROR, "expected parameter name",
                    tk.line, tk.column);
            sync(p);
            return -1;
        }
        p->tokens.data[p->cursor].type = TK_IDENT_VAR;
        pname_idx = new_ident(nodes, tk.value.string, ANODE_IDENT_VAR);
        p->cursor++;

        skip_newlines(p);
        if (curtok(p).type != TK_COLON) {
            diag_add(diags, LEVEL_ERROR, "expected ':' after parameter name",
                     curtok(p).line, curtok(p).column);
            sync(p);
            return -1;
        }
        p->cursor++;

        skip_newlines(p);
        tk = curtok(p);
        if (tk.type != TK_IDENT) {
            diag_add(diags, LEVEL_ERROR, "expected type name",
                    tk.line, tk.column);
            sync(p);
            return -1;
        }
        p->tokens.data[p->cursor].type = TK_IDENT_TYPE;
        if (!is_builtin_type(tk.value.string)) {
            diag_add(diags, LEVEL_ERROR, "unknown type name",
                    tk.line, tk.column);
            sync(p);
            return -1;
        }
        ptype_idx = new_ident(nodes, tk.value.string, ANODE_IDENT_TYPE);
        p->cursor++;

        vardecl_idx = new_node(nodes, ANODE_VARDECL);
        nodes->data[vardecl_idx].child = pname_idx;
        nodes->data[pname_idx].next = ptype_idx;

        if (param_head < 0)
            param_head = vardecl_idx;
        else
            nodes->data[param_tail].next = vardecl_idx;
        param_tail = vardecl_idx;

        skip_newlines(p);
        if (curtok(p).type == TK_COMMA) {
            p->cursor++;
        } else if (curtok(p).type != TK_CPAREN) {
            diag_add(diags, LEVEL_ERROR, "expected ',' or ')' after parameter",
                     curtok(p).line, curtok(p).column);
            sync(p);
            return -1;
        }
    }
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_MINUS) {
        diag_add(diags, LEVEL_ERROR, "expected '->' after parameters",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;
    skip_newlines(p);
    if (curtok(p).type != TK_CABRACKET) {
        diag_add(diags, LEVEL_ERROR, "expected '->' (missing '>' after '-')",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;

    skip_newlines(p);
    tk = curtok(p);
    if (tk.type != TK_IDENT) {
        diag_add(diags, LEVEL_ERROR, "expected return type after '->'",
                tk.line, tk.column);
        sync(p);
        return -1;
    }
    p->tokens.data[p->cursor].type = TK_IDENT_TYPE;
    if (!is_builtin_type(tk.value.string)) {
        diag_add(diags, LEVEL_ERROR, "unknown type name",
                tk.line, tk.column);
        sync(p);
        return -1;
    }
    rettype_idx = new_ident(nodes, tk.value.string, ANODE_IDENT_TYPE);
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_OBRACE) {
        diag_add(diags, LEVEL_ERROR, "expected '{' before function body",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;

    state_push(&p->states, PSTATE_BLOCK);
    initial_sz = p->states.size;
    {
        int block_idx = new_node(nodes, ANODE_BLOCK);
        int first_stmt = -1;
        int last_stmt = -1;

        while (p->states.size >= initial_sz && p->cursor < p->tokens.size) {
            skip_newlines(p);
            if (at_eof(p)) {
                diag_add(diags, LEVEL_ERROR, "unexpected end of file in function body",
                         curtok(p).line, curtok(p).column);
                break;
            }

            if (state_get(&p->states, p->states.size - 1) == PSTATE_BLOCK) {
                tk = curtok(p);
                if (tk.type == TK_CBRACE) {
                    p->cursor++;
                    state_pop(&p->states);
                    continue;
                }
                if (tk.type == TK_OBRACE) {
                    p->cursor++;
                    state_push(&p->states, PSTATE_BLOCK);
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

        func_idx = new_node(nodes, ANODE_FUNCDECL);
        nodes->data[func_idx].child = name_idx;

        prev_link = name_idx;
        if (param_head >= 0) {
            nodes->data[prev_link].next = param_head;
            prev_link = param_head;
            while (nodes->data[prev_link].next >= 0)
                prev_link = nodes->data[prev_link].next;
        }
        nodes->data[prev_link].next = rettype_idx;
        nodes->data[rettype_idx].next = block_idx;
    }

    return func_idx;
}

static int parse_return(parser *p, node_vector *nodes, diag_vector *diags)
{
    p->cursor++;
    (void)diags;
    return new_node(nodes, ANODE_RETURN);
}

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
                state_push(&p->states, PSTATE_BLOCK);
            } else {
                parse_stmt(p, nodes, diags);
            }
        } else {
            switch (state_get(&p->states, p->states.size - 1)) {
            case PSTATE_BLOCK: {
                token tk = curtok(p);
                if (tk.type == TK_CBRACE) {
                    p->cursor++;
                    state_pop(&p->states);
                } else if (tk.type == TK_OBRACE) {
                    p->cursor++;
                    state_push(&p->states, PSTATE_BLOCK);
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
