/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * sema_expr.c — Semantic analysis for expressions and assignments.
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

const Type *sema_expr(node_vector nodes, sym_set_vector *stack, int idx,
                      diag_vector *diags, sema_vector *annot)
{
    const anode *n;

    if (idx < 0)
        return NULL;

    n = &nodes.data[idx];

    switch (n->kind) {
    case ANODE_IDENT: {
        sym_entry *found =
            sym_set_find_bykey(cur_scope(stack), n->sv);
        if (!found) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "undeclared variable '%s'", n->sv);
            return NULL;
        }

        if (found->kind == SYM_CONST) {
            found->const_.is_used = true;
            annot->data[idx].type = found->const_.type;
            annot->data[idx].decl_idx = found->const_.ast_idx;
            return found->const_.type;
        }

        found->var.is_used = true;
        if (!found->var.is_assigned)
            diag_fmt(diags, LEVEL_WARN, 0, 0,
                     "variable '%s' used before assignment", n->sv);
        annot->data[idx].type = found->var.type;
        annot->data[idx].decl_idx = found->var.ast_idx;
        return found->var.type;
    }
    case ANODE_ILITERAL:
        annot->data[idx].type = TYPE_I32;
        return TYPE_I32;
    case ANODE_FLITERAL:
        annot->data[idx].type = TYPE_F64;
        return TYPE_F64;
    case ANODE_BINOP: {
        const Type *lt = sema_expr(nodes, stack, n->child, diags, annot);
        const Type *rt = NULL;
        const Type *common;
        bool is_cmp = false;
        if (n->child >= 0)
            rt = sema_expr(nodes, stack,
                           nodes.data[n->child].next, diags, annot);
        common = common_type(lt, rt);
        if (!common) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "type mismatch: cannot combine '%s' with '%s'",
                     lt ? lt->name : "?",
                     rt ? rt->name : "?");
            return NULL;
        }
        is_cmp = (n->op == TK_EQUAL || n->op == TK_NOTEQUAL
                  || n->op == TK_OABRACKET || n->op == TK_CABRACKET
                  || n->op == TK_LESSEQUAL || n->op == TK_GREATEREQUAL);
        annot->data[idx].type = is_cmp ? TYPE_BOOL : common;
        return is_cmp ? TYPE_BOOL : common;
    }
    case ANODE_UNOP: {
        const Type *t = sema_expr(nodes, stack, n->child, diags, annot);

        if (n->op == TK_NOT && !type_is_integer(t) && t != TYPE_BOOL) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "bitwise not requires integer operand, got '%s'",
                     t ? t->name : "?");
            return NULL;
        }

        annot->data[idx].type = t;
        return t;
    }
    case ANODE_STRUCTLIT: {
        const char *tname = n->sv;
        sym_entry *found = sym_set_find_bykey(cur_scope(stack), tname);
        const Type *sty = NULL;

        if (found && found->kind == SYM_TYPE)
            sty = found->type.type;
        if (!sty) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "unknown type '%s'", tname);
            return NULL;
        }
        if (sty->kind != TYPEK_STRUCT) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "'%s' is not a struct type", tname);
            return NULL;
        }

        {
            int fc = nodes.data[n->child].next;
            while (fc >= 0) {
                const anode *fi = &nodes.data[fc];
                const char *fn = fi->sv;
                int val_idx = fi->child;
                int i;
                const Type *fty = NULL;

                for (i = 0; i < sty->field_count; i++) {
                    if (strcmp(sty->field_names[i], fn) == 0) {
                        fty = sty->field_types[i];
                        break;
                    }
                }
                if (!fty) {
                    diag_fmt(diags, LEVEL_ERROR, 0, 0,
                             "struct '%s' has no field '%s'", tname, fn);
                } else {
                    const Type *vt = sema_expr(nodes, stack, val_idx,
                                                diags, annot);
                    if (vt && !assignable_to(fty, vt))
                        diag_fmt(diags, LEVEL_ERROR, 0, 0,
                                 "type mismatch: field '%s' of '%s' expects '%s',"
                                 " got '%s'",
                                 fn, tname,
                                 fty->name ? fty->name : "?",
                                 vt->name ? vt->name : "?");
                }
                fc = fi->next;
            }
        }

        annot->data[idx].type = sty;
        return sty;
    }
    case ANODE_ARRAYLIT: {
        const Type *elem_ty = NULL;
        int count = 0;
        int cur = n->child;
        bool mismatch = false;

        while (cur >= 0) {
            const Type *et = sema_expr(nodes, stack, cur, diags, annot);
            if (et) {
                if (!elem_ty) {
                    elem_ty = et;
                } else if (elem_ty != et) {
                    const Type *common = common_type(elem_ty, et);
                    if (!common) {
                        diag_fmt(diags, LEVEL_ERROR, 0, 0,
                                 "type mismatch: array element '%s' "
                                 "is incompatible with '%s'",
                                 elem_ty->name ? elem_ty->name : "?",
                                 et->name ? et->name : "?");
                        mismatch = true;
                    } else {
                        elem_ty = common;
                    }
                }
            }
            count++;
            cur = nodes.data[cur].next;
        }

        if (count == 0) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "cannot infer type of empty array literal");
            return NULL;
        }

        if (mismatch)
            return NULL;

        if (!elem_ty) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "cannot infer element type of array literal");
            return NULL;
        }

        {
            Type *arr_ty = type_array_new(elem_ty, count);
            annot->data[idx].type = arr_ty;
            return arr_ty;
        }
    }
    case ANODE_CALL: {
        const char *fname = nodes.data[n->child].sv;
        sym_entry *found = sym_set_find_bykey(cur_scope(stack), fname);
        int nargs = 0;
        int arg_idx;
        int pi;

        if (!found || found->kind != SYM_FUNC) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "undeclared function '%s'", fname);
            return NULL;
        }

        found->func.is_used = true;

        /* count args */
        arg_idx = nodes.data[n->child].next;
        while (arg_idx >= 0) {
            nargs++;
            arg_idx = nodes.data[arg_idx].next;
        }

        if (nargs != found->func.param_types.size) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "function '%s' expects %d arguments, got %d",
                     fname, found->func.param_types.size, nargs);
            return NULL;
        }

        /* check each arg type */
        arg_idx = nodes.data[n->child].next;
        for (pi = 0; pi < nargs && arg_idx >= 0; pi++) {
            const Type *expected = found->func.param_types.data[pi];
            const Type *actual = sema_expr(nodes, stack, arg_idx, diags, annot);
            if (actual && !assignable_to(expected, actual))
                diag_fmt(diags, LEVEL_ERROR, 0, 0,
                         "type mismatch: argument %d of '%s' expects '%s',"
                         " got '%s'",
                         pi + 1, fname,
                         expected->name ? expected->name : "?",
                         actual->name ? actual->name : "?");
            arg_idx = nodes.data[arg_idx].next;
        }

        annot->data[idx].type = found->func.ret_type;
        return found->func.ret_type;
    }
    case ANODE_FIELDACCESS: {
        const char *fn = n->sv;
        int obj_idx = n->child;
        int i;
        const Type *ot = sema_expr(nodes, stack, obj_idx, diags, annot);

        if (!ot) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "cannot access field '%s' on expression with no type", fn);
            return NULL;
        }
        if (ot->kind != TYPEK_STRUCT) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "cannot access field on non-struct type '%s'",
                     ot->name ? ot->name : "?");
            return NULL;
        }
        for (i = 0; i < ot->field_count; i++) {
            if (strcmp(ot->field_names[i], fn) == 0) {
                const Type *fty = ot->field_types[i];
                annot->data[idx].type = fty;
                return fty;
            }
        }
        diag_fmt(diags, LEVEL_ERROR, 0, 0,
                 "struct '%s' has no field '%s'",
                 ot->name ? ot->name : "?", fn);
        return NULL;
    }
    case ANODE_INDEX: {
        int arr_idx = n->child;
        int idx_idx = (arr_idx >= 0) ? nodes.data[arr_idx].next : -1;
        const Type *arr_ty = sema_expr(nodes, stack, arr_idx, diags, annot);

        if (!arr_ty) return NULL;
        if (arr_ty->kind != TYPEK_ARRAY) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "cannot index into non-array type '%s'",
                     arr_ty->name ? arr_ty->name : "?");
            return NULL;
        }

        if (idx_idx >= 0) {
            const Type *idx_ty = sema_expr(nodes, stack, idx_idx, diags, annot);
            if (idx_ty && !type_is_integer(idx_ty))
                diag_fmt(diags, LEVEL_WARN, 0, 0,
                         "array index should be integer, got '%s'",
                         idx_ty->name ? idx_ty->name : "?");
        }

        {
            const Type *elem_ty = arr_ty->elem_type;
            annot->data[idx].type = elem_ty;
            return elem_ty;
        }
    }
    default:
        return NULL;
    }
}

void sema_assign(node_vector nodes, sym_set_vector *stack, int idx,
                  diag_vector *diags, sema_vector *annot)
{
    const anode *asn = &nodes.data[idx];
    int target_idx = asn->child;
    const anode *target = &nodes.data[target_idx];
    const Type *rhs_type;

    if (target->kind == ANODE_FIELDACCESS) {
        const char *fn = target->sv;
        int obj_idx = target->child;
        int i;
        const Type *ot = sema_expr(nodes, stack, obj_idx, diags, annot);

        if (!ot || ot->kind != TYPEK_STRUCT) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "cannot assign to field of non-struct type");
            return;
        }
        for (i = 0; i < ot->field_count; i++) {
            if (strcmp(ot->field_names[i], fn) == 0) {
                const Type *fty = ot->field_types[i];
                annot->data[target_idx].type = fty;

                rhs_type = sema_expr(nodes, stack,
                                     nodes.data[target_idx].next,
                                     diags, annot);
                if (rhs_type && !assignable_to(fty, rhs_type))
                    diag_fmt(diags, LEVEL_ERROR, 0, 0,
                             "type mismatch: cannot assign '%s' "
                             "to field '%s' of type '%s'",
                             rhs_type->name ? rhs_type->name : "?",
                             fn,
                             fty->name ? fty->name : "?");
                return;
            }
        }
        diag_fmt(diags, LEVEL_ERROR, 0, 0,
                 "struct '%s' has no field '%s'",
                 ot->name ? ot->name : "?", fn);
        return;
    }

    if (target->kind == ANODE_INDEX) {
        int arr_idx = target->child;
        const Type *arr_ty = sema_expr(nodes, stack, arr_idx, diags, annot);

        if (!arr_ty || arr_ty->kind != TYPEK_ARRAY) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "cannot index-assign to non-array type");
            return;
        }

        {
            const Type *elem_ty = arr_ty->elem_type;
            annot->data[target_idx].type = elem_ty;

            rhs_type = sema_expr(nodes, stack,
                                 nodes.data[target_idx].next,
                                 diags, annot);
            if (rhs_type && !assignable_to(elem_ty, rhs_type))
                diag_fmt(diags, LEVEL_ERROR, 0, 0,
                         "type mismatch: cannot assign '%s' "
                         "to array element of type '%s'",
                         rhs_type->name ? rhs_type->name : "?",
                         elem_ty->name ? elem_ty->name : "?");
        }
        return;
    }

    {
        const char *name = target->sv;
        sym_entry *found = sym_set_find_bykey(cur_scope(stack), name);
        if (!found) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "assignment to undeclared variable '%s'", name);
            return;
        }

        /* Reject assignment to compile-time constants. */
        if (found->kind == SYM_CONST) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "cannot assign to constant '%s'", name);
            return;
        }

        found->var.is_assigned = true;

        /* Annotate the assignment target IDENT with its declaration. */
        annot->data[target_idx].type = found->var.type;
        annot->data[target_idx].decl_idx = found->var.ast_idx;

        rhs_type = sema_expr(nodes, stack, nodes.data[target_idx].next, diags,
                             annot);
        if (rhs_type && found->var.type) {
            if (!assignable_to(found->var.type, rhs_type))
                diag_fmt(diags, LEVEL_ERROR, 0, 0,
                         "type mismatch: cannot assign '%s' "
                         "to variable '%s' of type '%s'",
                         rhs_type->name ? rhs_type->name : "?",
                         name,
                         found->var.type->name ? found->var.type->name : "?");
        }
    }
}
