/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * sema_expr.c — Semantic analysis for expressions and assignments.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "sema/sema_internal.h"

type_tag sema_expr(node_vector nodes, sym_set_vector *stack, int idx,
                   diag_vector *diags, sema_vector *annot)
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
        return TYPE_VOID;
    case ANODE_BINOP: {
        type_tag lt = sema_expr(nodes, stack, n->child, diags, annot);
        type_tag rt = TYPE_VOID;
        type_tag common;
        if (n->child >= 0)
            rt = sema_expr(nodes, stack,
                           nodes.data[n->child].next, diags, annot);
        common = common_type(lt, rt);
        if (common == TYPE_COUNT) {
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "type mismatch: cannot combine '%s' with '%s'",
                     type_info_of(lt)->name,
                     type_info_of(rt)->name);
            return TYPE_VOID;
        }
        annot->data[idx].type = common;
        return common;
    }
    case ANODE_UNOP: {
        type_tag t = sema_expr(nodes, stack, n->child, diags, annot);
        annot->data[idx].type = t;
        return t;
    }
    default:
        return TYPE_VOID;
    }
}

void sema_assign(node_vector nodes, sym_set_vector *stack, int idx,
                  diag_vector *diags, sema_vector *annot)
{
    const anode *asn = &nodes.data[idx];
    int target_idx = asn->child;
    const anode *target = &nodes.data[target_idx];
    const char *name = target->sv;
    type_tag rhs_type;

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

    if (target_idx < 0)
        return;
    rhs_type = sema_expr(nodes, stack, nodes.data[target_idx].next, diags,
                         annot);
    if (rhs_type != TYPE_VOID && found->var.type != TYPE_VOID) {
        if (!assignable_to(found->var.type, rhs_type))
            diag_fmt(diags, LEVEL_ERROR, 0, 0,
                     "type mismatch: cannot assign '%s' "
                     "to variable '%s' of type '%s'",
                     type_info_of(rhs_type)->name,
                     name,
                     type_info_of(found->var.type)->name);
    }
}
