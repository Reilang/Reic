/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                                     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * sema.c — Semantic analysis: symbol tables, declaration checks, type inference.
 *
 * Walks the AST one function at a time.  Each function gets a fresh symbol
 * table (hash set keyed on variable name).  The traversal dispatches to
 * per-node-kind helpers, each of which is a stub for now.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "sema/sema.h"

#include "collect/hashset.h"
#include "type/type.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;   /* variable name (key) */
    type_tag type;      /* TYPE_I32, etc. */
    int ast_idx;        /* index of the ANODE_VARDECL node */
} sym_entry;

static uint64_t sym_hash(sym_entry e)
{
    return hash_str(e.name);
}

static bool sym_eq(sym_entry a, sym_entry b)
{
    return strcmp(a.name, b.name) == 0;
}

DECLARE_SET(sym_entry, sym)

static void sema_block(node_vector nodes, sym_set *sym, int block_idx,
                       diag_vector *diags);
static void sema_stmt(node_vector nodes, sym_set *sym, int idx,
                      diag_vector *diags);
static void sema_vardecl(node_vector nodes, sym_set *sym, int idx,
                         diag_vector *diags);
static void sema_assign(node_vector nodes, sym_set *sym, int idx,
                        diag_vector *diags);
static void sema_if(node_vector nodes, sym_set *sym, int idx,
                    diag_vector *diags);
static void sema_while(node_vector nodes, sym_set *sym, int idx,
                       diag_vector *diags);
static void sema_loop(node_vector nodes, sym_set *sym, int idx,
                      diag_vector *diags);
static void sema_return(node_vector nodes, sym_set *sym, int idx,
                        diag_vector *diags);

void sema_check(node_vector nodes, diag_vector *diags)
{
    int i;
    (void)diags; /* suppress unused warning until stubs are filled */

    for (i = 0; i < nodes.size; i++) {
        const anode *n = &nodes.data[i];

        if (n->kind != ANODE_FUNCDECL)
            continue;

        const anode *name_n = &nodes.data[n->child];
        sym_set sym;
        sym_set_new(&sym, 16, sym_hash, sym_eq);

        /* add parameters to symbol table */
        {
            int cur = name_n->next;
            while (cur >= 0 && nodes.data[cur].kind == ANODE_VARDECL) {
                const anode *vd = &nodes.data[cur];
                int var_idx = vd->child;
                int type_idx = nodes.data[var_idx].next;

                sym_entry entry;
                entry.name  = nodes.data[var_idx].sv;
                entry.type  = (type_tag)nodes.data[type_idx].iv;
                entry.ast_idx = cur;
                sym_set_insert(&sym, entry);

                cur = vd->next;
            }
        }

        /* traverse body */
        {
            /* the block is the last node in the name->... chain */
            int cur = name_n->next;
            int block_idx = -1;
            while (cur >= 0) {
                if (nodes.data[cur].kind == ANODE_BLOCK) {
                    block_idx = cur;
                    break;
                }
                cur = nodes.data[cur].next;
            }
            if (block_idx >= 0)
                sema_block(nodes, &sym, block_idx, diags);
        }

        sym_set_free(&sym);
    }
}

static void sema_block(node_vector nodes, sym_set *sym, int block_idx,
                       diag_vector *diags)
{
    int cur = nodes.data[block_idx].child;
    while (cur >= 0) {
        sema_stmt(nodes, sym, cur, diags);
        cur = nodes.data[cur].next;
    }
}

static void sema_stmt(node_vector nodes, sym_set *sym, int idx,
                      diag_vector *diags)
{
    anode *n = &nodes.data[idx];

    switch (n->kind) {
    case ANODE_VARDECL:  sema_vardecl(nodes, sym, idx, diags);  break;
    case ANODE_ASSIGN:   sema_assign(nodes, sym, idx, diags);   break;
    case ANODE_IF:       sema_if(nodes, sym, idx, diags);       break;
    case ANODE_WHILE:    sema_while(nodes, sym, idx, diags);    break;
    case ANODE_LOOP:     sema_loop(nodes, sym, idx, diags);     break;
    case ANODE_RETURN:   sema_return(nodes, sym, idx, diags);   break;
    default:                                                    break;
    }
    (void)diags;
}

static void sema_vardecl(node_vector nodes, sym_set *sym, int idx,
                         diag_vector *diags)
{
    (void)nodes;
    (void)sym;
    (void)idx;
    (void)diags;
    /* TODO: check duplicate name, add to sym table, infer type if omitted */
}

static void sema_assign(node_vector nodes, sym_set *sym, int idx,
                        diag_vector *diags)
{
    (void)nodes;
    (void)sym;
    (void)idx;
    (void)diags;
    /* TODO: check left-hand side is declared, check type compatibility */
}

static void sema_if(node_vector nodes, sym_set *sym, int idx,
                    diag_vector *diags)
{
    (void)nodes;
    (void)sym;
    (void)idx;
    (void)diags;
    /* TODO: walk scrutinee expr, walk each arm body */
}

static void sema_while(node_vector nodes, sym_set *sym, int idx,
                       diag_vector *diags)
{
    (void)nodes;
    (void)sym;
    (void)idx;
    (void)diags;
    /* TODO: walk condition expr, walk body block */
}

static void sema_loop(node_vector nodes, sym_set *sym, int idx,
                      diag_vector *diags)
{
    (void)nodes;
    (void)sym;
    (void)idx;
    (void)diags;
    /* TODO: walk body block */
}

static void sema_return(node_vector nodes, sym_set *sym, int idx,
                        diag_vector *diags)
{
    (void)nodes;
    (void)sym;
    (void)idx;
    (void)diags;
    /* TODO: check return type matches function signature */
}
