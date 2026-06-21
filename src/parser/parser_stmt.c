/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * parser_stmt.c — Statement parsing: dispatch, if, while, loop, return.
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

int parse_stmt(parser *p, node_vector *nodes, diag_vector *diags)
{
    token tk = curtok(p);

    if (tk.type == TK_KEYWORD) {
        if (strcmp(tk.value.string, "fn") == 0)
            return parse_funcdef(p, nodes, diags);
        if (strcmp(tk.value.string, "var") == 0)
            return parse_vardecl(p, nodes, diags);
        if (strcmp(tk.value.string, "if") == 0)
            return parse_if(p, nodes, diags);
        if (strcmp(tk.value.string, "while") == 0)
            return parse_while(p, nodes, diags);
        if (strcmp(tk.value.string, "loop") == 0)
            return parse_loop(p, nodes, diags);
        if (strcmp(tk.value.string, "return") == 0)
            return parse_return(p, nodes, diags);
    }

    if (tk.type == TK_IDENT) {
        /* Lookahead: '=' alone -> constdecl, ':' -> assign (:=) */
        int saved = p->cursor;
        p->cursor++;
        skip_newlines(p);
        token next = curtok(p);
        p->cursor = saved;
        if (next.type == TK_EQUAL)
            return parse_constdecl(p, nodes, diags);
        else
            return parse_assign(p, nodes, diags);
    }

    diag_add(diags, LEVEL_ERROR, "expected statement",
            tk.line, tk.col);
    sync(p);
    return -1;
}

int parse_if(parser *p, node_vector *nodes, diag_vector *diags)
{
    token tk;
    int scrutinee_idx, if_idx;
    int arm_head = -1;
    int arm_tail = -1;

    p->cursor++; /* skip 'if' */

    skip_newlines(p);
    if (curtok(p).type != TK_OPAREN) {
        diag_add(diags, LEVEL_ERROR, "expected '(' after 'if'",
                 curtok(p).line, curtok(p).col);
        sync(p);
        return -1;
    }
    p->cursor++;

    scrutinee_idx = parse_expr(p, nodes, diags, 0);
    if (scrutinee_idx < 0) return -1;

    if (curtok(p).type != TK_CPAREN) {
        diag_add(diags, LEVEL_ERROR, "expected ')' after if condition",
                 curtok(p).line, curtok(p).col);
        sync(p);
        return -1;
    }
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_OBRACE) {
        diag_add(diags, LEVEL_ERROR, "expected '{' before if body",
                 curtok(p).line, curtok(p).col);
        sync(p);
        return -1;
    }
    p->cursor++;

    while (curtok(p).type != TK_CBRACE && !at_eof(p)) {
        int pattern_idx, arm_idx;
        int body_head = -1;
        int body_tail = -1;

        skip_newlines(p);
        tk = curtok(p);

        if (tk.type == TK_CBRACE)
            break;

        if (!is_arm_op(tk.type)) {
            diag_add(diags, LEVEL_ERROR, "expected match arm operator",
                     tk.line, tk.col);
            sync(p);
            if (arm_head < 0) return -1;
            break;
        }
        int arm_op = read_arm_op(p);

        skip_newlines(p);
        pattern_idx = parse_expr(p, nodes, diags, 0);
        if (pattern_idx < 0) return -1;

        skip_newlines(p);
        /* '=>' */
        if (curtok(p).type != TK_EQUAL) {
            diag_add(diags, LEVEL_ERROR, "expected '=>' after match pattern",
                     curtok(p).line, curtok(p).col);
            sync(p);
            return -1;
        }
        p->cursor++;
        skip_newlines(p);
        if (curtok(p).type != TK_CABRACKET) {
            diag_add(diags, LEVEL_ERROR,
                     "expected '=>' (missing '>' after '=')",
                     curtok(p).line, curtok(p).col);
            sync(p);
            return -1;
        }
        p->cursor++;

        /* arm body: stmts until next arm operator or '}' */
        for (;;) {
            skip_newlines(p);
            tk = curtok(p);
            if (is_arm_op(tk.type) || tk.type == TK_CBRACE || at_eof(p))
                break;

            int sidx = parse_stmt(p, nodes, diags);
            if (sidx >= 0) {
                if (body_head < 0)
                    body_head = sidx;
                else
                    nodes->data[body_tail].next = sidx;
                body_tail = sidx;
            }
        }

        arm_idx = new_node(nodes, ANODE_MATCHARM);
        nodes->data[arm_idx].op = arm_op;
        nodes->data[arm_idx].child = pattern_idx;
        if (body_head >= 0)
            nodes->data[pattern_idx].next = body_head;

        if (arm_head < 0)
            arm_head = arm_idx;
        else
            nodes->data[arm_tail].next = arm_idx;
        arm_tail = arm_idx;
    }

    if (curtok(p).type == TK_CBRACE)
        p->cursor++;

    if_idx = new_node(nodes, ANODE_IF);
    nodes->data[if_idx].child = scrutinee_idx;
    if (arm_head >= 0)
        nodes->data[scrutinee_idx].next = arm_head;

    return if_idx;
}

int parse_while(parser *p, node_vector *nodes, diag_vector *diags)
{
    int cond_idx, block_idx, while_idx;

    p->cursor++; /* skip 'while' */

    skip_newlines(p);
    if (curtok(p).type != TK_OPAREN) {
        diag_add(diags, LEVEL_ERROR, "expected '(' after 'while'",
                 curtok(p).line, curtok(p).col);
        sync(p);
        return -1;
    }
    p->cursor++;

    cond_idx = parse_expr(p, nodes, diags, 0);
    if (cond_idx < 0) return -1;

    if (curtok(p).type != TK_CPAREN) {
        diag_add(diags, LEVEL_ERROR, "expected ')' after while condition",
                 curtok(p).line, curtok(p).col);
        sync(p);
        return -1;
    }
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_OBRACE) {
        diag_add(diags, LEVEL_ERROR, "expected '{' before while body",
                 curtok(p).line, curtok(p).col);
        sync(p);
        return -1;
    }
    p->cursor++;

    state_vec_push(&p->states, PSTATE_BLOCK);
    block_idx = parse_block_body(p, nodes, diags, "while body");

    while_idx = new_node(nodes, ANODE_WHILE);
    nodes->data[while_idx].child = cond_idx;
    nodes->data[cond_idx].next = block_idx;

    return while_idx;
}

int parse_loop(parser *p, node_vector *nodes, diag_vector *diags)
{
    int block_idx, loop_idx;

    p->cursor++; /* skip 'loop' */

    skip_newlines(p);
    if (curtok(p).type != TK_OBRACE) {
        diag_add(diags, LEVEL_ERROR, "expected '{' after 'loop'",
                 curtok(p).line, curtok(p).col);
        sync(p);
        return -1;
    }
    p->cursor++;

    state_vec_push(&p->states, PSTATE_BLOCK);
    block_idx = parse_block_body(p, nodes, diags, "loop body");

    loop_idx = new_node(nodes, ANODE_LOOP);
    nodes->data[loop_idx].child = block_idx;

    return loop_idx;
}

int parse_return(parser *p, node_vector *nodes, diag_vector *diags)
{
    p->cursor++;
    int ret_idx = new_node(nodes, ANODE_RETURN);
    int expr_idx = parse_expr(p, nodes, diags, 0);
    if (expr_idx >= 0)
        nodes->data[ret_idx].child = expr_idx;
    return ret_idx;
}
