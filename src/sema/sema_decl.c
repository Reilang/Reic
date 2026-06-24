/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * sema_decl.c — Semantic analysis for declarations: var, const.
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

static const Type *resolve_typename(node_vector nodes, sym_set_vector *stack,
                                    int idx)
{
    const anode *n;

    if (idx < 0) return NULL;
    n = &nodes.data[idx];

    if (n->kind == ANODE_IDENT_TYPE)
        return n->type_val;

    if (n->kind == ANODE_IDENT) {
        const char *name = n->sv;
        sym_entry *found = sym_set_find_bykey(cur_scope(stack), name);
        if (found && found->kind == SYM_TYPE)
            return found->type.type;
        return type_from_name(name);
    }

    return NULL;
}

void sema_structdef_reg(node_vector nodes, sym_set_vector *stack, int cd_idx,
                         diag_vector *diags, sema_vector *annot)
{
    const anode *cd = &nodes.data[cd_idx];
    int name_idx = cd->child;
    const anode *name_n = &nodes.data[name_idx];
    const char *name = name_n->sv;
    sym_set *scope = cur_scope(stack);

    int sdef_idx = nodes.data[name_idx].next;
    const anode *sdef = &nodes.data[sdef_idx];

    const char *fnames[64];
    const Type *ftypes[64];
    int nf = 0;
    int fcur = sdef->child;

    while (fcur >= 0 && nf < 64) {
        const anode *fld = &nodes.data[fcur];
        const char *fname = fld->sv;
        const Type *fty = resolve_typename(nodes, stack, fld->child);

        if (!fty) {
            const anode *tan = &nodes.data[fld->child];
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "unknown type '%s' for field '%s' of struct '%s'",
                     (tan->kind == ANODE_IDENT) ? tan->sv : "?",
                     fname, name);
            fcur = fld->next;
            continue;
        }

        fnames[nf] = fname;
        ftypes[nf] = fty;
        nf++;
        fcur = fld->next;
    }

    Type *sty = type_struct_new(name, fnames, ftypes, nf, true);

    annot->data[cd_idx].type = sty;
    annot->data[name_idx].type = sty;
    annot->data[name_idx].decl_idx = cd_idx;

    {
        sym_entry entry;
        entry.key = name;
        entry.kind = SYM_TYPE;
        entry.decl_depth = cur_depth(stack);
        entry.type.type = sty;
        sym_set_insert(scope, entry);
    }
}

void sema_vardecl(node_vector nodes, sym_set_vector *stack, int idx,
                   diag_vector *diags, sema_vector *annot)
{
    const anode *vd = &nodes.data[idx];
    int name_idx = vd->child;
    const anode *name_n = &nodes.data[name_idx];
    const char *name = name_n->sv;
    sym_set *scope = cur_scope(stack);

    /* Check duplicate in current scope. */
    {
        sym_entry *existing = sym_set_find_bykey(scope, name);
        if (existing && existing->decl_depth == cur_depth(stack)) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "duplicate variable '%s'", name);
            return;
        }
    }

    const Type *type = TYPE_VOID;
    int type_idx = -1;
    int init_idx = -1;

    {
        int cur = name_n->next;
        if (cur >= 0 && nodes.data[cur].kind == ANODE_IDENT_TYPE) {
            type = nodes.data[cur].type_val;
            type_idx = cur;
            cur = nodes.data[cur].next;
        }
        if (cur >= 0)
            init_idx = cur;
    }

    {
        const Type *init_type = TYPE_VOID;

        if (init_idx >= 0)
            init_type = sema_expr(nodes, stack, init_idx, diags, annot);

        if (type_idx < 0) {
            if (init_idx >= 0) {
                if (init_type)
                    type = init_type;
                else
                    diag_fmt(diags, LEVEL_ERROR, 0, 0,
                             "cannot infer type of variable '%s' "
                             "(initializer has no concrete type)", name);
            } else {
                diag_fmt(diags, LEVEL_ERROR, 0, 0,
                         "cannot infer type of variable '%s' "
                         "without initializer or type annotation", name);
            }
        } else {
            if (init_idx >= 0 && init_type) {
                if (!assignable_to(type, init_type))
                    diag_fmt(diags, LEVEL_ERROR, 0, 0,
                             "type mismatch: cannot assign '%s' "
                             "to variable '%s' of type '%s'",
                             init_type->name ? init_type->name : "?",
                             name,
                             type->name ? type->name : "?");
            }
        }
    }

    /* Annotate this vardecl with its resolved type. */
    annot->data[idx].type = type;

    /* Insert into current scope. */
    {
        sym_entry entry;
        entry.key = name;
        entry.kind = SYM_VAR;
        entry.decl_depth = cur_depth(stack);
        entry.var.type = type;
        entry.var.is_assigned = (init_idx >= 0);
        entry.var.is_used = false;
        entry.var.ast_idx = idx;
        sym_set_insert(scope, entry);
    }
}

void sema_constdecl(node_vector nodes, sym_set_vector *stack, int idx,
                     diag_vector *diags, sema_vector *annot)
{
    const anode *cd = &nodes.data[idx];
    int name_idx = cd->child;
    const anode *name_n = &nodes.data[name_idx];
    const char *name = name_n->sv;
    sym_set *scope = cur_scope(stack);

    /* Check duplicate in current scope. */
    {
        sym_entry *existing = sym_set_find_bykey(scope, name);
        if (existing && existing->decl_depth == cur_depth(stack)) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "duplicate declaration '%s'", name);
            return;
        }
    }

    int value_idx = nodes.data[name_idx].next;

    if (value_idx >= 0 && nodes.data[value_idx].kind == ANODE_STRUCTDEF) {
        sema_structdef_reg(nodes, stack, idx, diags, annot);
        return;
    }

    /* For v1: only integer literals are compile-time evaluable. */
    if (value_idx < 0 || nodes.data[value_idx].kind != ANODE_ILITERAL) {
        diag_fmt(diags, LEVEL_ERROR, 0, 0,
                 "constant '%s' must be initialized with an integer literal"
                 " or type definition",
                 name);
        return;
    }

    const Type *type = TYPE_I32;
    int64_t value = nodes.data[value_idx].iv;

    /* Annotate the CONSTDECL and its name child. */
    annot->data[idx].type = type;
    annot->data[name_idx].type = type;
    annot->data[name_idx].decl_idx = idx;

    /* Insert into current scope. */
    {
        sym_entry entry;
        entry.key = name;
        entry.kind = SYM_CONST;
        entry.decl_depth = cur_depth(stack);
        entry.const_.type = type;
        entry.const_.value = value;
        entry.const_.is_used = false;
        entry.const_.ast_idx = idx;
        sym_set_insert(scope, entry);
    }
}
