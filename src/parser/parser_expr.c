/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * parser_expr.c — Expression parsing: primary, binary, assignment, arm ops.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 * Copyright (C) 2026  LLLichlet
 *
 * This file is part of ReiC.
 *
 * ReiC is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * ReiC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with ReiC.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "parser/parser.h"
#include "parser/parser_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_primary(parser *p, node_vector *nodes, diag_vector *diags)
{
    token tk = curtok(p);

    if (tk.type == TK_OBRACKET)
        return parse_arraylit(p, nodes, diags);

    if (tk.type == TK_ILITER) {
        int idx = new_node(nodes, ANODE_ILITERAL);
        nodes->data[idx].iv = tk.value.integer;
        p->cursor++;
        return idx;
    }
    if (tk.type == TK_FLITER) {
        int idx = new_node(nodes, ANODE_FLITERAL);
        nodes->data[idx].fv = tk.value.float_;
        p->cursor++;
        return idx;
    }
    if (tk.type == TK_IDENT) {
        int idx = new_ident(nodes, tk.value.string, ANODE_IDENT);
        p->cursor++;

        /* postfix: call (args), struct literal { fields }, field access .name */
        for (;;) {
            tk = curtok(p);
            if (tk.type == TK_OPAREN) {
                idx = parse_call(p, nodes, diags, idx);
            } else if (tk.type == TK_OBRACKET) {
                idx = parse_index(p, nodes, diags, idx);
            } else if (tk.type == TK_OBRACE) {
                idx = parse_structlit(p, nodes, diags, idx);
            } else if (tk.type == TK_DOT) {
                idx = parse_fieldaccess(p, nodes, diags, idx);
            } else {
                break;
            }
        }
        return idx;
    }
    if (tk.type == TK_OPAREN) {
        p->cursor++;
        skip_newlines(p);
        int idx = parse_expr(p, nodes, diags, 0);
        skip_newlines(p);
        if (curtok(p).type != TK_CPAREN) {
            diag_add(diags, LEVEL_ERROR, "expected ')'",
                     curtok(p).line, curtok(p).col);
            sync(p);
            return idx >= 0 ? idx : -1;
        }
        p->cursor++;
        return idx;
    }

    diag_add(diags, LEVEL_ERROR, "expected expression",
             tk.line, tk.col);
    sync(p);
    return -1;
}

int parse_call(parser *p, node_vector *nodes, diag_vector *diags, int callee_idx)
{
    int call_idx = new_node(nodes, ANODE_CALL);
    int first_arg = -1;
    int last_arg = -1;

    p->cursor++;  /* skip '(' */

    while (curtok(p).type != TK_CPAREN && !at_eof(p)) {
        int arg_idx;

        skip_newlines(p);
        if (curtok(p).type == TK_CPAREN) break;

        arg_idx = parse_expr(p, nodes, diags, 0);
        if (arg_idx < 0) break;

        if (first_arg < 0)
            first_arg = arg_idx;
        else
            nodes->data[last_arg].next = arg_idx;
        last_arg = arg_idx;

        skip_newlines(p);
        if (curtok(p).type == TK_CPAREN) break;
        if (curtok(p).type != TK_COMMA) {
            diag_add(diags, LEVEL_ERROR, "expected ',' or ')' in argument list",
                     curtok(p).line, curtok(p).col);
            sync(p);
            break;
        }
        p->cursor++;  /* skip ',' */
    }

    if (curtok(p).type == TK_CPAREN)
        p->cursor++;
    else
        diag_add(diags, LEVEL_ERROR, "expected ')' to close argument list",
                 curtok(p).line, curtok(p).col);

    nodes->data[call_idx].child = callee_idx;
    nodes->data[callee_idx].next = first_arg;
    return call_idx;
}

int parse_structlit(parser *p, node_vector *nodes, diag_vector *diags,
                    int type_idx)
{
    int lit_idx = new_node(nodes, ANODE_STRUCTLIT);
    int first_field = -1;
    int last_field = -1;

    /* keep type name on structlit for downstream, link IDENT as first child */
    nodes->data[lit_idx].sv = nodes->data[type_idx].sv;
    nodes->data[lit_idx].child = type_idx;

    p->cursor++;  /* skip '{' */

    while (curtok(p).type != TK_CBRACE && !at_eof(p)) {
        token ftk;
        int val_idx, finit_idx;

        skip_newlines(p);
        if (curtok(p).type == TK_CBRACE) break;

        ftk = curtok(p);
        if (ftk.type != TK_IDENT) {
            diag_add(diags, LEVEL_ERROR, "expected field name",
                     ftk.line, ftk.col);
            sync(p);
            break;
        }
        p->cursor++;

        skip_newlines(p);
        if (curtok(p).type != TK_COLON) {
            diag_add(diags, LEVEL_ERROR, "expected ':' after field name",
                     curtok(p).line, curtok(p).col);
            sync(p);
            break;
        }
        p->cursor++;
        skip_newlines(p);

        val_idx = parse_expr(p, nodes, diags, 0);
        if (val_idx < 0) break;

        finit_idx = new_node(nodes, ANODE_FIELDINIT);
        nodes->data[finit_idx].sv = strdup(ftk.value.string);
        nodes->data[finit_idx].child = val_idx;

        if (first_field < 0)
            first_field = finit_idx;
        else
            nodes->data[last_field].next = finit_idx;
        last_field = finit_idx;

        skip_newlines(p);
        if (curtok(p).type == TK_CBRACE) break;
        if (curtok(p).type != TK_COMMA) {
            diag_add(diags, LEVEL_ERROR, "expected ',' or '}' after field",
                     curtok(p).line, curtok(p).col);
            sync(p);
            break;
        }
        p->cursor++;  /* skip ',' */
    }

    if (curtok(p).type == TK_CBRACE)
        p->cursor++;

    nodes->data[type_idx].next = first_field;
    return lit_idx;
}

int parse_fieldaccess(parser *p, node_vector *nodes, diag_vector *diags,
                      int expr_idx)
{
    int fa_idx;

    p->cursor++;  /* skip '.' */
    skip_newlines(p);

    {
        token tk = curtok(p);
        if (tk.type != TK_IDENT) {
            diag_add(diags, LEVEL_ERROR, "expected field name after '.'",
                     tk.line, tk.col);
            sync(p);
            return expr_idx;
        }
        p->cursor++;

        fa_idx = new_node(nodes, ANODE_FIELDACCESS);
        nodes->data[fa_idx].sv = strdup(tk.value.string);
        nodes->data[fa_idx].child = expr_idx;
    }

    return fa_idx;
}

int parse_expr(parser *p, node_vector *nodes, diag_vector *diags, int min_prec)
{
    int left = parse_primary(p, nodes, diags);
    if (left < 0)
        return -1;

    for (;;) {
        token tk = curtok(p);
        int prec;

        switch (tk.type) {
        case TK_OABRACKET:
        case TK_CABRACKET: prec = 1; break;
        case TK_ADD:
        case TK_MINUS:     prec = 2; break;
        case TK_STAR:
        case TK_SLASH:     prec = 3; break;
        default:           prec = 0; break;
        }
        if (prec <= min_prec)
            break;

        tktype op = tk.type;
        p->cursor++;

        int right = parse_expr(p, nodes, diags, prec);
        if (right < 0)
            return left;

        int binop = new_node(nodes, ANODE_BINOP);
        nodes->data[binop].op = op;
        nodes->data[binop].child = left;
        nodes->data[left].next = right;
        left = binop;
    }

    return left;
}

int parse_assign(parser *p, node_vector *nodes, diag_vector *diags)
{
    int target_idx, expr_idx, assign_idx;

    /* parse target as a general expression:  x  or  obj.x */
    target_idx = parse_expr(p, nodes, diags, 0);
    if (target_idx < 0) return -1;

    {
        anode_kind tk = nodes->data[target_idx].kind;
        if (tk != ANODE_IDENT && tk != ANODE_FIELDACCESS && tk != ANODE_INDEX) {
            diag_add(diags, LEVEL_ERROR, "invalid assignment target",
                     curtok(p).line, curtok(p).col);
            sync(p);
            return target_idx;
        }
    }

    skip_newlines(p);
    if (curtok(p).type != TK_COLON) {
        diag_add(diags, LEVEL_ERROR, "expected ':=' in assignment",
                 curtok(p).line, curtok(p).col);
        sync(p);
        return -1;
    }
    p->cursor++;
    skip_newlines(p);
    if (curtok(p).type != TK_EQUAL) {
        diag_add(diags, LEVEL_ERROR, "expected ':=' in assignment (missing '=' after ':')",
                 curtok(p).line, curtok(p).col);
        sync(p);
        return -1;
    }
    p->cursor++;
    skip_newlines(p);

    expr_idx = parse_expr(p, nodes, diags, 0);
    if (expr_idx < 0) return -1;

    assign_idx = new_node(nodes, ANODE_ASSIGN);
    nodes->data[assign_idx].child = target_idx;
    nodes->data[target_idx].next = expr_idx;

    return assign_idx;
}

int is_arm_op(tktype t)
{
    switch (t) {
    case TK_EQUAL:
    case TK_OABRACKET:
    case TK_CABRACKET:
    case TK_NOT:
        return 1;
    default:
        return 0;
    }
}

/*
 * Read a match-arm comparison operator.
 * Handles compound operators >=  <=  !=
 * Returns the operator code (tktype) to store in MATCHARM.op.
 */
int read_arm_op(parser *p)
{
    tktype base = curtok(p).type;

    if (p->cursor + 1 < p->tokens.size) {
        token next = token_vec_get(&p->tokens, p->cursor + 1);
        if (next.type == TK_EQUAL) {
            p->cursor += 2;
            switch (base) {
            case TK_CABRACKET: return TK_GREATEREQUAL;
            case TK_OABRACKET: return TK_LESSEQUAL;
            case TK_NOT:       return TK_NOTEQUAL;
            default:           break;
            }
        }
    }

    p->cursor++;
    return (int)base;
}

int parse_arraylit(parser *p, node_vector *nodes, diag_vector *diags)
{
    int lit_idx = new_node(nodes, ANODE_ARRAYLIT);
    int first_elem = -1;
    int last_elem = -1;

    p->cursor++; /* skip '[' */

    while (curtok(p).type != TK_CBRACKET && !at_eof(p)) {
        skip_newlines(p);
        if (curtok(p).type == TK_CBRACKET) break;

        int elem_idx = parse_expr(p, nodes, diags, 0);
        if (elem_idx < 0) break;

        if (first_elem < 0)
            first_elem = elem_idx;
        else
            nodes->data[last_elem].next = elem_idx;
        last_elem = elem_idx;

        skip_newlines(p);
        if (curtok(p).type == TK_CBRACKET) break;
        if (curtok(p).type != TK_COMMA) {
            diag_add(diags, LEVEL_ERROR, "expected ',' or ']' in array literal",
                     curtok(p).line, curtok(p).col);
            break;
        }
        p->cursor++; /* skip ',' */
    }

    if (curtok(p).type == TK_CBRACKET)
        p->cursor++;
    else
        diag_add(diags, LEVEL_ERROR, "expected ']' to close array literal",
                 curtok(p).line, curtok(p).col);

    nodes->data[lit_idx].child = first_elem;
    return lit_idx;
}

int parse_index(parser *p, node_vector *nodes, diag_vector *diags, int target_idx)
{
    int idx_node = new_node(nodes, ANODE_INDEX);

    p->cursor++; /* skip '[' */
    skip_newlines(p);

    int idx_expr = parse_expr(p, nodes, diags, 0);
    if (idx_expr < 0) {
        if (curtok(p).type == TK_CBRACKET) p->cursor++;
        return target_idx;
    }

    skip_newlines(p);
    if (curtok(p).type != TK_CBRACKET)
        diag_add(diags, LEVEL_ERROR, "expected ']' after index",
                 curtok(p).line, curtok(p).col);
    else
        p->cursor++;

    nodes->data[idx_node].child = target_idx;
    nodes->data[target_idx].next = idx_expr;
    return idx_node;
}
