/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * sema.c — Semantic analysis entry point and shared helpers.
 *
 * sema_check() walks the AST after parsing, builds per-function symbol tables,
 * checks for undeclared variables, infers types, and writes diagnostics.
 *
 * Scope management helpers (scope_enter / scope_leave) encapsulate the
 * copy-push / exit-pop-free pattern used by sema_block and sema_if.
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

/* hash/eq functions used with sym_set_new in sema_check */
static uint64_t sema_sym_hash(sym_entry e)
{
    return hash_str(e.key);
}

static bool sema_sym_eq(sym_entry a, sym_entry b)
{
    return strcmp(a.key, b.key) == 0;
}

sym_set *cur_scope(sym_set_vector *stack)
{
    return &stack->data[stack->size - 1];
}

int cur_depth(const sym_set_vector *stack)
{
    return stack->size - 1;
}

/* Deep copy a sym_set. */
void sym_set_copy(sym_set *dst, const sym_set *src)
{
    int i;
    dst->cap = src->cap;
    dst->size = src->size;
    dst->hash_fn = src->hash_fn;
    dst->eq_fn = src->eq_fn;

    dst->entries =
        (sym_entry *)malloc((size_t)dst->cap * sizeof(sym_entry));
    if (!dst->entries) abort();
    dst->occupied = (bool *)malloc((size_t)dst->cap * sizeof(bool));
    if (!dst->occupied) abort();

    memcpy(dst->entries, src->entries,
           (size_t)dst->cap * sizeof(sym_entry));
    memcpy(dst->occupied, src->occupied,
           (size_t)dst->cap * sizeof(bool));

    for (i = 0; i < dst->cap; i++) {
        if (dst->occupied[i] && dst->entries[i].kind == SYM_FUNC) {
            type_ptr_vector *pt = &dst->entries[i].func.param_types;
            if (pt->cap > 0 && pt->data) {
                Type **new_data =
                    (Type **)malloc((size_t)pt->cap * sizeof(Type *));
                if (!new_data) abort();
                memcpy(new_data, pt->data,
                       (size_t)pt->size * sizeof(Type *));
                pt->data = new_data;
            }
        }
    }
}

/* Push a new scope that deep-copies the current top scope. */
void scope_enter(sym_set_vector *stack)
{
    sym_set new_scope;
    sym_set_copy(&new_scope, cur_scope(stack));
    sym_set_vec_push(stack, new_scope);
}

/*
 * Propagate used/assigned flags to the parent scope for inherited entries,
 * warn about unused locals, then pop and free the top scope.
 */
static void scope_exit(sym_set_vector *stack, diag_vector *diags)
{
    sym_set *inner = cur_scope(stack);
    int depth = cur_depth(stack);
    int i;
    sym_set *parent = NULL;

    if (stack->size >= 2)
        parent = &stack->data[stack->size - 2];

    for (i = 0; i < inner->cap; i++) {
        if (!inner->occupied[i])
            continue;

        sym_entry *e = &inner->entries[i];

        if (e->kind == SYM_VAR) {
            if (e->decl_depth < depth) {
                if (parent) {
                    sym_entry *pe =
                        sym_set_find_bykey(parent, e->key);
                    if (pe && pe->kind == SYM_VAR) {
                        pe->var.is_used |= e->var.is_used;
                        pe->var.is_assigned |= e->var.is_assigned;
                    }
                }
            } else {
                if (!e->var.is_used)
                    diag_fmt(diags, LEVEL_WARN, 0, 0,
                             "unused variable '%s'", e->key);
            }
        } else if (e->kind == SYM_CONST) {
            if (e->decl_depth < depth) {
                if (parent) {
                    sym_entry *pe =
                        sym_set_find_bykey(parent, e->key);
                    if (pe && pe->kind == SYM_CONST) {
                        pe->const_.is_used |= e->const_.is_used;
                    }
                }
            } else {
                if (!e->const_.is_used)
                    diag_fmt(diags, LEVEL_WARN, 0, 0,
                             "unused constant '%s'", e->key);
            }
        }
    }
}

void scope_leave(sym_set_vector *stack, diag_vector *diags)
{
    scope_exit(stack, diags);
    sym_set top = sym_set_vec_pop(stack);
    sym_set_free(&top);
}

void diag_fmt(diag_vector *diags, level lv, int line, int col,
              const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    diag_add(diags, lv, buf, line, col);
}

const Type *common_type(const Type *a, const Type *b)
{
    if (!a || !b) return NULL;
    if (a == b) return a;

    /* Integer: same signedness, wider wins. */
    if (type_is_integer(a) && type_is_integer(b)) {
        if (type_is_signed(a) != type_is_signed(b)) return NULL;
        return (type_width(a) >= type_width(b)) ? a : b;
    }

    /* Float: wider wins. */
    if (type_is_float(a) && type_is_float(b))
        return (type_width(a) >= type_width(b)) ? a : b;

    return NULL;
}

bool assignable_to(const Type *dst, const Type *src)
{
    if (!dst || !src) return false;
    if (dst == src) return true;

    /* Integer: same signedness, dst wider. */
    if (type_is_integer(dst) && type_is_integer(src)) {
        if (type_is_signed(dst) != type_is_signed(src)) return false;
        return type_width(dst) >= type_width(src);
    }

    /* Float: dst wider or same width. */
    if (type_is_float(dst) && type_is_float(src))
        return type_width(dst) >= type_width(src);

    return false;
}

sema_vector sema_check(node_vector nodes, diag_vector *diags)
{
    int i;
    sym_set_vector stack;
    sema_vector annot;
    sym_set_vec_new(&stack, 8);

    /* Pre-allocate annotation vector: one entry per AST node. */
    sema_vec_new(&annot, nodes.size);
    for (i = 0; i < nodes.size; i++) {
        sema_annot a = { .type = NULL, .decl_idx = -1 };
        sema_vec_push(&annot, a);
    }

    for (i = 0; i < nodes.size; i++) {
        const anode *n = &nodes.data[i];

        if (n->kind != ANODE_FUNCDECL)
            continue;

        const anode *name_n = &nodes.data[n->child];
        sym_set func_scope;
        sym_set_new(&func_scope, 16, sema_sym_hash, sema_sym_eq);

        /* Find return type (skip params first). */
        {
            int cur = name_n->next;
            while (cur >= 0 && nodes.data[cur].kind == ANODE_VARDECL)
                cur = nodes.data[cur].next;
            if (cur >= 0 && nodes.data[cur].kind == ANODE_IDENT_TYPE)
                annot.data[i].type = nodes.data[cur].type_val;
        }

        /* Add parameters to function scope. */
        {
            int cur = name_n->next;
            while (cur >= 0 && nodes.data[cur].kind == ANODE_VARDECL) {
                const anode *vd = &nodes.data[cur];
                int var_idx = vd->child;
                int type_idx = nodes.data[var_idx].next;

                sym_entry entry;
                entry.key = nodes.data[var_idx].sv;
                entry.kind = SYM_VAR;
                entry.decl_depth = 0;
                entry.var.is_assigned = true;
                entry.var.is_used = false;
                entry.var.type = nodes.data[type_idx].type_val;
                entry.var.ast_idx = cur;
                sym_set_insert(&func_scope, entry);

                /* Annotate parameter VARDECL. */
                annot.data[cur].type = nodes.data[type_idx].type_val;

                cur = vd->next;
            }
        }

        sym_set_vec_push(&stack, func_scope);

        /* Process function body block. */
        {
            int cur = name_n->next;
            int block_idx = -1;
            while (cur >= 0) {
                if (nodes.data[cur].kind == ANODE_BLOCK) {
                    block_idx = cur;
                    break;
                }
                cur = nodes.data[cur].next;
            }
            if (block_idx >= 0) {
                int stmt = nodes.data[block_idx].child;
                while (stmt >= 0) {
                    sema_stmt(nodes, &stack, stmt, diags, &annot);
                    stmt = nodes.data[stmt].next;
                }
            }
        }

        /* Check unused in function scope. */
        scope_leave(&stack, diags);
    }

    sym_set_vec_free(&stack);
    return annot;
}
