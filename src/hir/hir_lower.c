/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * hir_lower.c — AST -> HIR lowering: walks AST, consults sema annotations,
 * builds a typed HIR with resolved IDENT references and implicit casts.
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
#include "hir/hir_lower.h"

#include <stdlib.h>
#include <string.h>

/* Forward: lower a single AST node, return its HIR index.
 * expected != NULL means the caller wants the result at that type,
 * so an implicit HIR_CAST is inserted when the expression's own type
 * differs but is narrower. */
static int lower_node(node_vector nodes, const sema_vector *annot,
                      hir_vector *hir, int *ast2hir,
                      int idx, const Type *expected);

static int lower_expr(node_vector nodes, const sema_vector *annot,
                      hir_vector *hir, int *ast2hir,
                      int idx, const Type *expected)
{
    return lower_node(nodes, annot, hir, ast2hir, idx, expected);
}

static int lower_stmt(node_vector nodes, const sema_vector *annot,
                      hir_vector *hir, int *ast2hir,
                      int idx)
{
    return lower_node(nodes, annot, hir, ast2hir, idx, NULL);
}

/* Push a new hnode, return its index. */
static int hir_push(hir_vector *hir, hnode n)
{
    hir_vec_push(hir, n);
    return hir->size - 1;
}

/* Insert a HIR_CAST between a lowered expression and its parent when the
 * expression's type differs from the expected type. */
static int maybe_cast(hir_vector *hir, int inner_hir_idx,
                      const Type *inner_type, const Type *expected)
{
    if (!expected || inner_type == expected)
        return inner_hir_idx;

    hnode cast;
    cast.kind = HIR_CAST;
    cast.type = expected;
    cast.child = inner_hir_idx;
    cast.next = -1;
    return hir_push(hir, cast);
}

/* Lower fn parameters (ANODE_VARDECL siblings before the body block).
 * Returns the last lowered param's HIR index so the caller can link .next
 * to the body. */
static int lower_params(node_vector nodes, const sema_vector *annot,
                        hir_vector *hir, int *ast2hir,
                        int first_param_idx)
{
    int prev = -1;
    int first = -1;
    int cur = first_param_idx;
    while (cur >= 0 && nodes.data[cur].kind == ANODE_VARDECL) {
        int p = lower_node(nodes, annot, hir, ast2hir, cur, NULL);
        if (prev >= 0)
            hir->data[prev].next = p;
        else
            first = p;
        prev = p;
        cur = nodes.data[cur].next;
    }
    (void)first;
    return prev;
}

/* Lower a block's statements (ANODE_BLOCK.child linked via .next).
 * Returns the first lowered statement's HIR index, or -1 if empty. */
static int lower_block_body(node_vector nodes, const sema_vector *annot,
                            hir_vector *hir, int *ast2hir,
                            int block_idx)
{
    if (block_idx < 0 || nodes.data[block_idx].kind != ANODE_BLOCK)
        return -1;

    int first = -1;
    int prev = -1;
    int cur = nodes.data[block_idx].child;
    while (cur >= 0) {
        int s = lower_stmt(nodes, annot, hir, ast2hir, cur);
        if (prev >= 0)
            hir->data[prev].next = s;
        else
            first = s;
        prev = s;
        cur = nodes.data[cur].next;
    }
    return first;
}

static int lower_node(node_vector nodes, const sema_vector *annot,
                      hir_vector *hir, int *ast2hir,
                      int idx, const Type *expected)
{
    if (idx < 0)
        return -1;

    const anode *n = &nodes.data[idx];
    hnode hn;
    memset(&hn, 0, sizeof(hn));
    hn.kind = HIR_NONE;
    hn.type = annot->data[idx].type;
    hn.child = -1;
    hn.next = -1;

    switch (n->kind) {
    case ANODE_FUNCDECL: {
        int name_idx = n->child;
        hn.kind = HIR_FUNCDECL;
        hn.sv = nodes.data[name_idx].sv;
        hn.type = annot->data[idx].type;

        int cur = nodes.data[name_idx].next;
        int body_idx = -1;
        int first_param = cur;

        while (cur >= 0) {
            if (nodes.data[cur].kind == ANODE_BLOCK) {
                body_idx = cur;
                break;
            }
            cur = nodes.data[cur].next;
        }

        int param_last = -1;
        if (first_param >= 0 && first_param != body_idx
            && nodes.data[first_param].kind == ANODE_VARDECL) {
            param_last = lower_params(nodes, annot, hir, ast2hir, first_param);
        }

        int body_hir = lower_node(nodes, annot, hir, ast2hir,
                                    body_idx, NULL);

        if (param_last >= 0)
            hir->data[param_last].next = body_hir;
        hn.child = (first_param >= 0 && first_param != body_idx
                    && nodes.data[first_param].kind == ANODE_VARDECL)
                       ? ast2hir[first_param]
                       : body_hir;
        break;
    }
    case ANODE_VARDECL: {
        int name_idx = n->child;
        hn.kind = HIR_VARDECL;
        hn.sv = nodes.data[name_idx].sv;

        int cur = name_idx;
        int init_idx = -1;
        cur = nodes.data[cur].next;
        if (cur >= 0 && nodes.data[cur].kind == ANODE_IDENT_TYPE)
            cur = nodes.data[cur].next;
        if (cur >= 0)
            init_idx = cur;

        if (init_idx >= 0) {
            int init_hir = lower_expr(nodes, annot, hir, ast2hir,
                                      init_idx, hn.type);
            hn.child = init_hir;
        }
        break;
    }
    case ANODE_BLOCK:
        hn.kind = HIR_BLOCK;
        hn.child = lower_block_body(nodes, annot, hir, ast2hir, idx);
        break;
    case ANODE_ASSIGN: {
        hn.kind = HIR_ASSIGN;
        int target_idx = n->child;
        int target_hir = lower_expr(nodes, annot, hir, ast2hir,
                                    target_idx, NULL);
        hn.child = target_hir;

        if (target_idx >= 0) {
            int rhs_idx = nodes.data[target_idx].next;
            if (rhs_idx >= 0) {
                const Type *target_type = annot->data[target_idx].type;
                int rhs_hir = lower_expr(nodes, annot, hir, ast2hir,
                                         rhs_idx, target_type);
                hir->data[target_hir].next = rhs_hir;
            }
        }
        break;
    }
    case ANODE_RETURN:
        hn.kind = HIR_RETURN;
        if (n->child >= 0)
            hn.child = lower_expr(nodes, annot, hir, ast2hir,
                                  n->child, expected);
        break;
    case ANODE_IF: {
        hn.kind = HIR_IF;
        int scrutinee_idx = n->child;
        if (scrutinee_idx < 0)
            break;
        int scrutinee_hir = lower_expr(nodes, annot, hir, ast2hir,
                                       scrutinee_idx, NULL);
        hn.child = scrutinee_hir;

        int prev_arm = scrutinee_hir;
        int arm = nodes.data[scrutinee_idx].next;
        while (arm >= 0) {
            const anode *arm_n = &nodes.data[arm];
            if (arm_n->kind != ANODE_MATCHARM) {
                arm = arm_n->next;
                continue;
            }
            int arm_hir = lower_node(nodes, annot, hir, ast2hir,
                                     arm, NULL);
            hir->data[prev_arm].next = arm_hir;
            prev_arm = arm_hir;
            arm = arm_n->next;
        }
        break;
    }
    case ANODE_MATCHARM: {
        hn.kind = HIR_MATCHARM;
        hn.op = n->op;
        int pattern_idx = n->child;
        if (pattern_idx >= 0) {
            int pattern_hir = lower_expr(nodes, annot, hir, ast2hir,
                                         pattern_idx, NULL);
            hn.child = pattern_hir;
            int prev = pattern_hir;
            int body = nodes.data[pattern_idx].next;
            while (body >= 0) {
                int body_hir = lower_stmt(nodes, annot, hir, ast2hir, body);
                hir->data[prev].next = body_hir;
                prev = body_hir;
                body = nodes.data[body].next;
            }
        }
        break;
    }
    case ANODE_WHILE: {
        hn.kind = HIR_WHILE;
        int cond_idx = n->child;
        if (cond_idx >= 0) {
            int cond_hir = lower_expr(nodes, annot, hir, ast2hir,
                                      cond_idx, NULL);
            hn.child = cond_hir;
            int body_idx = nodes.data[cond_idx].next;
            if (body_idx >= 0) {
                int body_hir = lower_node(nodes, annot, hir, ast2hir,
                                          body_idx, NULL);
                hir->data[cond_hir].next = body_hir;
            }
        }
        break;
    }
    case ANODE_LOOP:
        hn.kind = HIR_LOOP;
        if (n->child >= 0)
            hn.child = lower_node(nodes, annot, hir, ast2hir,
                                  n->child, NULL);
        break;
    case ANODE_ILITERAL:
        hn.kind = HIR_ILITERAL;
        hn.iv = n->iv;
        break;
    case ANODE_FLITERAL:
        hn.kind = HIR_FLITERAL;
        hn.fv = n->fv;
        break;
    case ANODE_SLITERAL:
        hn.kind = HIR_SLITERAL;
        hn.sv = n->sv;
        break;
    case ANODE_CLITERAL:
        hn.kind = HIR_CLITERAL;
        hn.cv = n->cv;
        break;
    case ANODE_BINOP: {
        hn.kind = HIR_BINOP;
        hn.op = n->op;
        const Type *result_type = annot->data[idx].type;
        int lhs_idx = n->child;
        if (lhs_idx >= 0) {
            int rhs_idx = nodes.data[lhs_idx].next;
            /*
             * For comparison operators the result type is bool, but
             * operands share their own common type.  Derive it from
             * the operand annotations.
             */
            bool is_cmp = (n->op == TK_EQUAL || n->op == TK_NOTEQUAL
                           || n->op == TK_OABRACKET || n->op == TK_CABRACKET
                           || n->op == TK_LESSEQUAL || n->op == TK_GREATEREQUAL);
            const Type *op_expected = result_type;
            if (is_cmp) {
                const Type *lt = annot->data[lhs_idx].type;
                const Type *rt = (rhs_idx >= 0) ? annot->data[rhs_idx].type : NULL;
                op_expected = lt;
                if (rt && type_width(rt) > type_width(lt))
                    op_expected = rt;
            }
            int lhs_hir = lower_expr(nodes, annot, hir, ast2hir,
                                     lhs_idx, op_expected);
            hn.child = lhs_hir;
            if (rhs_idx >= 0) {
                int rhs_hir = lower_expr(nodes, annot, hir, ast2hir,
                                         rhs_idx, op_expected);
                hir->data[lhs_hir].next = rhs_hir;
            }
        }
        break;
    }
    case ANODE_UNOP:
        hn.kind = HIR_UNOP;
        hn.op = n->op;
        if (n->child >= 0)
            hn.child = lower_expr(nodes, annot, hir, ast2hir,
                                  n->child, hn.type);
        break;
    case ANODE_IDENT_VAR:
    case ANODE_IDENT: {
        int decl_ast = annot->data[idx].decl_idx;
        if (decl_ast >= 0
            && nodes.data[decl_ast].kind == ANODE_CONSTDECL) {
            /* Inline compile-time constant as a literal. */
            int name_idx = nodes.data[decl_ast].child;
            int value_idx = nodes.data[name_idx].next;
            hn.type = annot->data[idx].type;
            if (nodes.data[value_idx].kind == ANODE_FLITERAL) {
                hn.kind = HIR_FLITERAL;
                hn.fv = nodes.data[value_idx].fv;
            } else {
                hn.kind = HIR_ILITERAL;
                hn.iv = nodes.data[value_idx].iv;
            }
        } else {
            hn.kind = HIR_IDENT;
            if (decl_ast >= 0 && ast2hir[decl_ast] >= 0)
                hn.iv = ast2hir[decl_ast];
            else
                hn.iv = -1;
        }
        break;
    }
    case ANODE_CALL: {
        hn.kind = HIR_CALL;
        hn.type = annot->data[idx].type;

        int callee_idx = n->child;
        hn.sv = nodes.data[callee_idx].sv;

        int first_arg = -1;
        int prev_arg = -1;
        int arg = nodes.data[callee_idx].next;
        while (arg >= 0) {
            int arg_hir = lower_expr(nodes, annot, hir, ast2hir, arg, NULL);
            if (first_arg < 0)
                first_arg = arg_hir;
            else
                hir->data[prev_arg].next = arg_hir;
            prev_arg = arg_hir;
            arg = nodes.data[arg].next;
        }
        hn.child = first_arg;
        break;
    }
    case ANODE_INDEX:
        hn.kind = HIR_NONE;
        break;
    case ANODE_STRUCTLIT: {
        hn.kind = HIR_STRUCTLIT;
        hn.type = annot->data[idx].type;

        int first_val = -1;
        int prev_val = -1;
        int fc = nodes.data[n->child].next;
        while (fc >= 0) {
            const anode *fi = &nodes.data[fc];
            int val_idx = fi->child;
            const Type *fty = NULL;
            int i;

            if (hn.type) {
                for (i = 0; i < hn.type->field_count; i++) {
                    if (strcmp(hn.type->field_names[i], fi->sv) == 0) {
                        fty = hn.type->field_types[i];
                        break;
                    }
                }
            }
            int vhir = lower_expr(nodes, annot, hir, ast2hir, val_idx, fty);
            if (first_val < 0)
                first_val = vhir;
            else
                hir->data[prev_val].next = vhir;
            prev_val = vhir;
            fc = fi->next;
        }
        hn.child = first_val;
        break;
    }
    case ANODE_FIELDACCESS: {
        hn.kind = HIR_FIELDACCESS;
        hn.type = annot->data[idx].type;
        hn.sv = n->sv;
        if (n->child >= 0)
            hn.child = lower_expr(nodes, annot, hir, ast2hir,
                                  n->child, NULL);
        break;
    }
    case ANODE_CONSTDECL:
        /* Const values are inlined at use sites; decl itself is no-op. */
        hn.kind = HIR_NONE;
        break;
    default:
        hn.kind = HIR_NONE;
        break;
    }

    int hir_idx = hir_push(hir, hn);
    ast2hir[idx] = hir_idx;

    if (expected != NULL && hn.type != expected && hn.type != NULL) {
        int casted = maybe_cast(hir, hir_idx, hn.type, expected);
        return casted;
    }

    return hir_idx;
}

hir_vector hir_lower(node_vector nodes, const sema_vector *annot)
{
    hir_vector hir;
    hir_vec_new(&hir, nodes.size);

    int *ast2hir = (int *)malloc((size_t)nodes.size * sizeof(int));
    if (!ast2hir) abort();
    for (int i = 0; i < nodes.size; i++)
        ast2hir[i] = -1;

    for (int i = 0; i < nodes.size; i++) {
        if (nodes.data[i].kind == ANODE_FUNCDECL)
            lower_node(nodes, annot, &hir, ast2hir, i, NULL);
    }

    free(ast2hir);
    return hir;
}
