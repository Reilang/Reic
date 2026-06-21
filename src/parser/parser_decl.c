/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * parser_decl.c — Declaration parsing: fn, var, const.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "parser/parser.h"
#include "parser/parser_internal.h"
#include "type/type.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_funcdef(parser *p, node_vector *nodes, diag_vector *diags)
{
    token tk;
    int name_idx, rettype_idx, func_idx;
    int param_head = -1;
    int param_tail = -1;
    int prev_link;

    p->cursor++; /* skip 'fn' */

    skip_newlines(p);
    tk = curtok(p);
    if (tk.type != TK_IDENT) {
        diag_add(diags, LEVEL_ERROR, "expected function name after 'fn'",
                tk.line, tk.col);
        sync(p);
        return -1;
    }
    name_idx = new_ident(nodes, tk.value.string, ANODE_IDENT_FUNC);
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_OPAREN) {
        diag_add(diags, LEVEL_ERROR, "expected '(' after function name",
                 curtok(p).line, curtok(p).col);
        sync(p);
        return -1;
    }
    p->cursor++;

    while (curtok(p).type != TK_CPAREN) {
        int pname_idx, ptype_idx, vardecl_idx;

        if (at_eof(p)) {
            diag_add(diags, LEVEL_ERROR, "expected ')' to close parameter list",
                     tk.line, tk.col);
            sync(p);
            return -1;
        }

        tk = curtok(p);
        if (tk.type != TK_IDENT) {
            diag_add(diags, LEVEL_ERROR, "expected parameter name",
                    tk.line, tk.col);
            sync(p);
            return -1;
        }
        pname_idx = new_ident(nodes, tk.value.string, ANODE_IDENT_VAR);
        p->cursor++;

        if (curtok(p).type != TK_COLON) {
            diag_add(diags, LEVEL_ERROR, "expected ':' after parameter name",
                     curtok(p).line, curtok(p).col);
            sync(p);
            return -1;
        }
        p->cursor++;

        tk = curtok(p);
        if (tk.type != TK_IDENT) {
            diag_add(diags, LEVEL_ERROR, "expected type name",
                    tk.line, tk.col);
            sync(p);
            return -1;
        }
        {
            type_tag tag = type_from_name(tk.value.string);
            if (tag == TYPE_COUNT) {
                diag_add(diags, LEVEL_ERROR, "unknown type name",
                        tk.line, tk.col);
                sync(p);
                return -1;
            }
            ptype_idx = new_node(nodes, ANODE_IDENT_TYPE);
            nodes->data[ptype_idx].iv = tag;
        }
        p->cursor++;

        vardecl_idx = new_node(nodes, ANODE_VARDECL);
        nodes->data[vardecl_idx].child = pname_idx;
        nodes->data[pname_idx].next = ptype_idx;

        if (param_head < 0)
            param_head = vardecl_idx;
        else
            nodes->data[param_tail].next = vardecl_idx;
        param_tail = vardecl_idx;

        if (curtok(p).type == TK_COMMA) {
            p->cursor++;
        } else if (curtok(p).type != TK_CPAREN) {
            diag_add(diags, LEVEL_ERROR, "expected ',' or ')' after parameter",
                     curtok(p).line, curtok(p).col);
            sync(p);
            return -1;
        }
    }
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_MINUS) {
        diag_add(diags, LEVEL_ERROR, "expected '->' after parameters",
                 curtok(p).line, curtok(p).col);
        sync(p);
        return -1;
    }
    p->cursor++;
    skip_newlines(p);
    if (curtok(p).type != TK_CABRACKET) {
        diag_add(diags, LEVEL_ERROR, "expected '->' (missing '>' after '-')",
                 curtok(p).line, curtok(p).col);
        sync(p);
        return -1;
    }
    p->cursor++;

    skip_newlines(p);
    tk = curtok(p);
    if (tk.type != TK_IDENT) {
        diag_add(diags, LEVEL_ERROR, "expected return type after '->'",
                tk.line, tk.col);
        sync(p);
        return -1;
    }
    {
        type_tag tag = type_from_name(tk.value.string);
        if (tag == TYPE_COUNT) {
            diag_add(diags, LEVEL_ERROR, "unknown type name",
                    tk.line, tk.col);
            sync(p);
            return -1;
        }
        rettype_idx = new_node(nodes, ANODE_IDENT_TYPE);
        nodes->data[rettype_idx].iv = tag;
    }
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_OBRACE) {
        diag_add(diags, LEVEL_ERROR, "expected '{' before function body",
                 curtok(p).line, curtok(p).col);
        sync(p);
        return -1;
    }
    p->cursor++;

    state_vec_push(&p->states, PSTATE_BLOCK);
    {
        int block_idx = parse_block_body(p, nodes, diags, "function body");

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

int parse_vardecl(parser *p, node_vector *nodes, diag_vector *diags)
{
    token tk;
    int name_idx, type_idx, init_idx, vardecl_idx;
    int prev;

    p->cursor++; /* skip 'var' */

    skip_newlines(p);
    tk = curtok(p);
    if (tk.type != TK_IDENT) {
        diag_add(diags, LEVEL_ERROR, "expected variable name after 'var'",
                tk.line, tk.col);
        sync(p);
        return -1;
    }

    name_idx = new_ident(nodes, tk.value.string, ANODE_IDENT_VAR);
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_COLON) {
        diag_add(diags, LEVEL_ERROR, "expected ':' after variable name",
                 curtok(p).line, curtok(p).col);
        sync(p);
        return -1;
    }
    p->cursor++;
    skip_newlines(p);

    type_idx = -1;
    init_idx = -1;

    if (curtok(p).type == TK_EQUAL) {
        /* var x := expr */
        p->cursor++;
        skip_newlines(p);
        init_idx = parse_expr(p, nodes, diags, 0);
        if (init_idx < 0) return -1;
    } else if (curtok(p).type == TK_IDENT) {
        /* var x: type */
        tk = curtok(p);
        {
            type_tag tag = type_from_name(tk.value.string);
            if (tag == TYPE_COUNT) {
                diag_add(diags, LEVEL_ERROR, "unknown type name",
                        tk.line, tk.col);
                sync(p);
                return -1;
            }
            type_idx = new_node(nodes, ANODE_IDENT_TYPE);
            nodes->data[type_idx].iv = tag;
        }
        p->cursor++;

        skip_newlines(p);
        /* optionally: `:= expr` */
        if (curtok(p).type == TK_COLON) {
            p->cursor++;
            skip_newlines(p);
            if (curtok(p).type != TK_EQUAL) {
                diag_add(diags, LEVEL_ERROR, "expected ':=' after type",
                         curtok(p).line, curtok(p).col);
                sync(p);
                return -1;
            }
            p->cursor++;
            skip_newlines(p);
            init_idx = parse_expr(p, nodes, diags, 0);
            if (init_idx < 0) return -1;
        }
    } else {
        diag_add(diags, LEVEL_ERROR, "expected type name or ':='",
                 curtok(p).line, curtok(p).col);
        sync(p);
        return -1;
    }

    vardecl_idx = new_node(nodes, ANODE_VARDECL);
    nodes->data[vardecl_idx].child = name_idx;

    prev = name_idx;
    if (type_idx >= 0) {
        nodes->data[prev].next = type_idx;
        prev = type_idx;
    }
    if (init_idx >= 0) {
        nodes->data[prev].next = init_idx;
    }

    return vardecl_idx;
}

int parse_constdecl(parser *p, node_vector *nodes, diag_vector *diags)
{
    token tk = curtok(p);
    int name_idx, value_idx, constdecl_idx;

    /* tk is TK_IDENT -- confirmed by caller */
    name_idx = new_ident(nodes, tk.value.string, ANODE_IDENT_VAR);
    p->cursor++;

    skip_newlines(p);
    p->cursor++;  /* skip '=' */
    skip_newlines(p);

    value_idx = parse_expr(p, nodes, diags, 0);
    if (value_idx < 0) return -1;

    constdecl_idx = new_node(nodes, ANODE_CONSTDECL);
    nodes->data[constdecl_idx].child = name_idx;
    nodes->data[name_idx].next = value_idx;

    return constdecl_idx;
}
