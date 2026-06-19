/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                                     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * sema.c — Semantic analysis: symbol tables, declaration checks, type inference.
 *
 * Uses a scope stack (sym_set_vector) where each scope is a deep copy of its
 * parent.  Lookup only searches the top scope.  On scope exit, inherited
 * entries propagate is_used / is_assigned back to the parent, then local
 * entries are checked for unused warnings.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "sema/sema.h"

#include "collect/hashset.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t sym_hash(sym_entry e)
{
    return hash_str(e.key);
}

static bool sym_eq(sym_entry a, sym_entry b)
{
    return strcmp(a.key, b.key) == 0;
}

DECLARE_SET(sym_entry, sym)
DECLARE_VECTOR(sym_set, sym_set)

/* Return pointer to the top sym_set of the scope stack. */
static sym_set *cur_scope(sym_set_vector *stack)
{
    return &stack->data[stack->size - 1];
}

/* Scope depth of the top scope (function scope = 0). */
static int cur_depth(const sym_set_vector *stack)
{
    return stack->size - 1;
}

/* Deep copy a sym_set.  Allocates new entries / occupied arrays and copies
 * param_types vectors for function symbols. */
static void sym_set_copy(sym_set *dst, const sym_set *src)
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
            type_tag_vector *pt = &dst->entries[i].func.param_types;
            if (pt->cap > 0 && pt->data) {
                type_tag *new_data =
                    (type_tag *)malloc((size_t)pt->cap * sizeof(type_tag));
                if (!new_data) abort();
                memcpy(new_data, pt->data,
                       (size_t)pt->size * sizeof(type_tag));
                pt->data = new_data;
            }
        }
    }
}

/* Formatted diagnostic helper. */
static void diag_fmt(diag_vector *diags, level lv, int line, int col,
                     const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    diag_add(diags, lv, buf, line, col);
}

/* Two types are compatible in an expression if they share the same signedness
 * family (both signed or both unsigned).  Returns the wider type, or TYPE_COUNT
 * if they are incompatible (cross-family or involving void). */
static type_tag common_type(type_tag a, type_tag b)
{
    const type_info *ai, *bi;

    if (a == b)
        return a;
    ai = type_info_of(a);
    bi = type_info_of(b);
    if (ai->is_signed != bi->is_signed)
        return TYPE_COUNT;
    if (a == TYPE_VOID || b == TYPE_VOID)
        return TYPE_COUNT;
    return (ai->width >= bi->width) ? a : b;
}

/* Can src be assigned to a location of type dst without explicit cast?
 * Returns true for same type or implicit widening (same family, dst wider). */
static bool assignable_to(type_tag dst, type_tag src)
{
    const type_info *di, *si;

    if (dst == src)
        return true;
    if (dst == TYPE_VOID || src == TYPE_VOID)
        return false;
    di = type_info_of(dst);
    si = type_info_of(src);
    if (di->is_signed != si->is_signed)
        return false;
    return di->width >= si->width;
}

/* forward declarations */
static void sema_block(node_vector nodes, sym_set_vector *stack,
                       int block_idx, diag_vector *diags);
static void sema_stmt(node_vector nodes, sym_set_vector *stack, int idx,
                      diag_vector *diags);
static type_tag sema_expr(node_vector nodes, sym_set_vector *stack, int idx,
                            diag_vector *diags);
static void sema_vardecl(node_vector nodes, sym_set_vector *stack, int idx,
                         diag_vector *diags);
static void sema_assign(node_vector nodes, sym_set_vector *stack, int idx,
                        diag_vector *diags);
static void sema_if(node_vector nodes, sym_set_vector *stack, int idx,
                    diag_vector *diags);
static void sema_while(node_vector nodes, sym_set_vector *stack, int idx,
                       diag_vector *diags);
static void sema_loop(node_vector nodes, sym_set_vector *stack, int idx,
                      diag_vector *diags);
static void sema_return(node_vector nodes, sym_set_vector *stack, int idx,
                        diag_vector *diags);

/* Propagate is_used / is_assigned from inner scope entries back to parent,
 * then check unused for locally-declared entries. */
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
        if (e->kind != SYM_VAR)
            continue;

        if (e->decl_depth < depth) {
            /* Inherited — propagate flags back to parent. */
            if (parent) {
                sym_entry *pe =
                    sym_set_find_bykey(parent, e->key);
                if (pe && pe->kind == SYM_VAR) {
                    pe->var.is_used |= e->var.is_used;
                    pe->var.is_assigned |= e->var.is_assigned;
                }
            }
        } else {
            /* Locally declared — check unused. */
            if (!e->var.is_used)
                diag_fmt(diags, LEVEL_WARN, 0, 0,
                         "unused variable '%s'", e->key);
        }
    }
}

void sema_check(node_vector nodes, diag_vector *diags)
{
    int i;
    sym_set_vector stack;
    sym_set_vec_new(&stack, 8);

    for (i = 0; i < nodes.size; i++) {
        const anode *n = &nodes.data[i];

        if (n->kind != ANODE_FUNCDECL)
            continue;

        const anode *name_n = &nodes.data[n->child];
        sym_set func_scope;
        sym_set_new(&func_scope, 16, sym_hash, sym_eq);

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
                entry.var.type = (type_tag)nodes.data[type_idx].iv;
                sym_set_insert(&func_scope, entry);

                cur = vd->next;
            }
        }

        sym_set_vec_push(&stack, func_scope);

        /* Process function body statements in the function scope itself
         * (no new scope push — params and body are the same scope). */
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
                    sema_stmt(nodes, &stack, stmt, diags);
                    stmt = nodes.data[stmt].next;
                }
            }
        }

        /* Check unused and pop function scope. */
        {
            sym_set *scope = cur_scope(&stack);
            int j;
            for (j = 0; j < scope->cap; j++) {
                if (!scope->occupied[j])
                    continue;
                sym_entry *e = &scope->entries[j];
                if (e->kind == SYM_VAR && !e->var.is_used)
                    diag_fmt(diags, LEVEL_WARN, 0, 0,
                             "unused variable '%s'", e->key);
            }
        }

        {
            sym_set top = sym_set_vec_pop(&stack);
            sym_set_free(&top);
        }
    }

    sym_set_vec_free(&stack);
}

static void sema_block(node_vector nodes, sym_set_vector *stack,
                       int block_idx, diag_vector *diags)
{
    sym_set new_scope;
    sym_set_copy(&new_scope, cur_scope(stack));
    sym_set_vec_push(stack, new_scope);

    int cur = nodes.data[block_idx].child;
    while (cur >= 0) {
        sema_stmt(nodes, stack, cur, diags);
        cur = nodes.data[cur].next;
    }

    scope_exit(stack, diags);

    {
        sym_set top = sym_set_vec_pop(stack);
        sym_set_free(&top);
    }
}

static void sema_stmt(node_vector nodes, sym_set_vector *stack, int idx,
                      diag_vector *diags)
{
    anode *n = &nodes.data[idx];

    switch (n->kind) {
    case ANODE_BLOCK:
        sema_block(nodes, stack, idx, diags);
        break;
    case ANODE_VARDECL:
        sema_vardecl(nodes, stack, idx, diags);
        break;
    case ANODE_ASSIGN:
        sema_assign(nodes, stack, idx, diags);
        break;
    case ANODE_IF:
        sema_if(nodes, stack, idx, diags);
        break;
    case ANODE_WHILE:
        sema_while(nodes, stack, idx, diags);
        break;
    case ANODE_LOOP:
        sema_loop(nodes, stack, idx, diags);
        break;
    case ANODE_RETURN:
        sema_return(nodes, stack, idx, diags);
        break;
    default:
        break;
    }
}

static type_tag sema_expr(node_vector nodes, sym_set_vector *stack, int idx,
                            diag_vector *diags)
{
    const anode *n;

    if (idx < 0)
        return TYPE_VOID;

    n = &nodes.data[idx];

    switch (n->kind) {
    case ANODE_IDENT: {
        sym_entry *found =
            sym_set_find_bykey(cur_scope(stack), n->sv);
        if (!found) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "undeclared variable '%s'", n->sv);
            return TYPE_VOID;
        }
        found->var.is_used = true;
        if (!found->var.is_assigned)
            diag_fmt(diags, LEVEL_WARN, 0, 0,
                     "variable '%s' used before assignment", n->sv);
        return found->var.type;
    }
    case ANODE_ILITERAL:
        return TYPE_I32;
    case ANODE_FLITERAL:
        return TYPE_VOID;
    case ANODE_BINOP: {
        type_tag lt = sema_expr(nodes, stack, n->child, diags);
        type_tag rt = TYPE_VOID;
        type_tag common;
        if (n->child >= 0)
            rt = sema_expr(nodes, stack,
                          nodes.data[n->child].next, diags);
        common = common_type(lt, rt);
        if (common == TYPE_COUNT) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "type mismatch: cannot combine '%s' with '%s'",
                     type_info_of(lt)->name,
                     type_info_of(rt)->name);
            return TYPE_VOID;
        }
        return common;
    }
    case ANODE_UNOP:
        return sema_expr(nodes, stack, n->child, diags);
    default:
        /* literals, ANODE_CALL, ANODE_INDEX: not yet typed */
        return TYPE_VOID;
    }
}

static void sema_vardecl(node_vector nodes, sym_set_vector *stack, int idx,
                         diag_vector *diags)
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

    /* Determine type and optional init expression. */
    type_tag type = TYPE_VOID;
    int type_idx = -1;
    int init_idx = -1;

    {
        int cur = name_n->next;
        if (cur >= 0 && nodes.data[cur].kind == ANODE_IDENT_TYPE) {
            type = (type_tag)nodes.data[cur].iv;
            type_idx = cur;
            cur = nodes.data[cur].next;
        }
        if (cur >= 0)
            init_idx = cur;
    }

    /* Walk init expression BEFORE inserting the variable, so self-references
     * in the initializer resolve to the outer variable (if any). */
    {
        type_tag init_type = TYPE_VOID;

        if (init_idx >= 0)
            init_type = sema_expr(nodes, stack, init_idx, diags);

        if (type_idx < 0) {
            /* No type annotation — infer from initializer. */
            if (init_idx >= 0) {
                if (init_type != TYPE_VOID)
                    type = init_type;
                else
                    diag_fmt(diags, LEVEL_ERROR, 0, 0,
                             "cannot infer type of variable '%s' (initializer has no concrete type)", name);
            } else {
                diag_fmt(diags, LEVEL_ERROR, 0, 0,
                         "cannot infer type of variable '%s' without initializer or type annotation", name);
            }
        } else {
            /* Type annotation present — check that init can widen into it. */
            if (init_idx >= 0 && init_type != TYPE_VOID) {
                if (!assignable_to(type, init_type))
                    diag_fmt(diags, LEVEL_ERROR, 0, 0,
                             "type mismatch: cannot assign '%s' to variable '%s' of type '%s'",
                             type_info_of(init_type)->name,
                             name,
                             type_info_of(type)->name);
            }
        }
    }

    /* Insert into current scope. */
    {
        sym_entry entry;
        entry.key = name;
        entry.kind = SYM_VAR;
        entry.decl_depth = cur_depth(stack);
        entry.var.type = type;
        entry.var.is_assigned = (init_idx >= 0);
        entry.var.is_used = false;
        sym_set_insert(scope, entry);
    }

}

static void sema_assign(node_vector nodes, sym_set_vector *stack, int idx,
                        diag_vector *diags)
{
    const anode *asn = &nodes.data[idx];
    int target_idx = asn->child;
    const anode *target = &nodes.data[target_idx];
    const char *name = target->sv;
    type_tag rhs_type;

    /* Look up the variable being assigned. */
    sym_entry *found = sym_set_find_bykey(cur_scope(stack), name);
    if (!found) {
        diag_fmt(diags, LEVEL_ERROR, 0, 0,
                 "assignment to undeclared variable '%s'", name);
        return;
    }
    found->var.is_assigned = true;

    /* Walk the right-hand side expression and check type compatibility. */
    if (target_idx < 0)
        return;
    rhs_type = sema_expr(nodes, stack, nodes.data[target_idx].next, diags);
    if (rhs_type != TYPE_VOID && found->var.type != TYPE_VOID) {
        if (!assignable_to(found->var.type, rhs_type))
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "type mismatch: cannot assign '%s' to variable '%s' of type '%s'",
                     type_info_of(rhs_type)->name,
                     name,
                     type_info_of(found->var.type)->name);
    }
}

static void sema_if(node_vector nodes, sym_set_vector *stack, int idx,
                    diag_vector *diags)
{
    const anode *n = &nodes.data[idx];
    int scrutinee_idx = n->child;

    /* Walk scrutinee expression. */
    sema_expr(nodes, stack, scrutinee_idx, diags);

    /* Walk arm bodies (each arm body is a block, linked via .next). */
    int arm = nodes.data[scrutinee_idx].next;
    while (arm >= 0) {
        const anode *arm_n = &nodes.data[arm];
        if (arm_n->kind == ANODE_MATCHARM) {
            /* matcharm: child=pattern, pattern.next=body block */
            int body_idx = arm_n->child;
            if (body_idx >= 0)
                body_idx = nodes.data[body_idx].next;
            if (body_idx >= 0)
                sema_block(nodes, stack, body_idx, diags);
        }
        arm = arm_n->next;
    }
}

static void sema_while(node_vector nodes, sym_set_vector *stack, int idx,
                       diag_vector *diags)
{
    const anode *n = &nodes.data[idx];
    int cond_idx = n->child;

    /* Walk condition expression. */
    sema_expr(nodes, stack, cond_idx, diags);

    /* Walk body block. */
    if (cond_idx >= 0) {
        int body_idx = nodes.data[cond_idx].next;
        if (body_idx >= 0)
            sema_block(nodes, stack, body_idx, diags);
    }
}

static void sema_loop(node_vector nodes, sym_set_vector *stack, int idx,
                      diag_vector *diags)
{
    const anode *n = &nodes.data[idx];
    int body_idx = n->child;

    if (body_idx >= 0)
        sema_block(nodes, stack, body_idx, diags);
}

static void sema_return(node_vector nodes, sym_set_vector *stack, int idx,
                        diag_vector *diags)
{
    const anode *n = &nodes.data[idx];

    /* Walk returned expression if present. */
    sema_expr(nodes, stack, n->child, diags);
}
