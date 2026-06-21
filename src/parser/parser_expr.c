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
    token tk = curtok(p);
    int var_idx, expr_idx, assign_idx;

    var_idx = new_ident(nodes, tk.value.string, ANODE_IDENT_VAR);
    p->cursor++;

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
    nodes->data[assign_idx].child = var_idx;
    nodes->data[var_idx].next = expr_idx;

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
