/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * sema_stmt.c — Semantic analysis for statements: block, if, while, loop, return.
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
#include "sema/sema_internal.h"

void sema_block(node_vector nodes, sym_set_vector *stack,
                 int block_idx, diag_vector *diags, sema_vector *annot)
{
    scope_enter(stack);

    int cur = nodes.data[block_idx].child;
    while (cur >= 0) {
        sema_stmt(nodes, stack, cur, diags, annot);
        cur = nodes.data[cur].next;
    }

    scope_leave(stack, diags);
}

void sema_stmt(node_vector nodes, sym_set_vector *stack, int idx,
                diag_vector *diags, sema_vector *annot)
{
    anode *n = &nodes.data[idx];

    switch (n->kind) {
    case ANODE_BLOCK:
        sema_block(nodes, stack, idx, diags, annot);
        break;
    case ANODE_VARDECL:
        sema_vardecl(nodes, stack, idx, diags, annot);
        break;
    case ANODE_CONSTDECL:
        sema_constdecl(nodes, stack, idx, diags, annot);
        break;
    case ANODE_ASSIGN:
        sema_assign(nodes, stack, idx, diags, annot);
        break;
    case ANODE_IF:
        sema_if(nodes, stack, idx, diags, annot);
        break;
    case ANODE_WHILE:
        sema_while(nodes, stack, idx, diags, annot);
        break;
    case ANODE_LOOP:
        sema_loop(nodes, stack, idx, diags, annot);
        break;
    case ANODE_RETURN:
        sema_return(nodes, stack, idx, diags, annot);
        break;
    default:
        break;
    }
}

void sema_if(node_vector nodes, sym_set_vector *stack, int idx,
              diag_vector *diags, sema_vector *annot)
{
    const anode *n = &nodes.data[idx];
    int scrutinee_idx = n->child;

    sema_expr(nodes, stack, scrutinee_idx, diags, annot);

    int arm = nodes.data[scrutinee_idx].next;
    while (arm >= 0) {
        const anode *arm_n = &nodes.data[arm];
        if (arm_n->kind == ANODE_MATCHARM) {
            /* child = pattern, pattern.next = body stmts (not a BLOCK) */
            int pattern_idx = arm_n->child;
            if (pattern_idx >= 0) {
                sema_expr(nodes, stack, pattern_idx, diags, annot);

                /* Push new scope for arm body. */
                scope_enter(stack);

                int stmt = nodes.data[pattern_idx].next;
                while (stmt >= 0) {
                    sema_stmt(nodes, stack, stmt, diags, annot);
                    stmt = nodes.data[stmt].next;
                }

                scope_leave(stack, diags);
            }
        }
        arm = arm_n->next;
    }
}

void sema_while(node_vector nodes, sym_set_vector *stack, int idx,
                 diag_vector *diags, sema_vector *annot)
{
    const anode *n = &nodes.data[idx];
    int cond_idx = n->child;

    sema_expr(nodes, stack, cond_idx, diags, annot);

    if (cond_idx >= 0) {
        int body_idx = nodes.data[cond_idx].next;
        if (body_idx >= 0)
            sema_block(nodes, stack, body_idx, diags, annot);
    }
}

void sema_loop(node_vector nodes, sym_set_vector *stack, int idx,
                diag_vector *diags, sema_vector *annot)
{
    const anode *n = &nodes.data[idx];
    int body_idx = n->child;

    if (body_idx >= 0)
        sema_block(nodes, stack, body_idx, diags, annot);
}

void sema_return(node_vector nodes, sym_set_vector *stack, int idx,
                  diag_vector *diags, sema_vector *annot)
{
    const anode *n = &nodes.data[idx];
    sema_expr(nodes, stack, n->child, diags, annot);
}
