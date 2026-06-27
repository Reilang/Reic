/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * hir.c — HIR node utility: debug string conversion and ASCII tree printing.
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
#include "hir/hir.h"
#include "collect/strbuf.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *kind_name(hkind kind)
{
    switch (kind) {
    case HIR_NONE:         return "NONE";
    case HIR_FUNCDECL:     return "FUNCDECL";
    case HIR_VARDECL:      return "VARDECL";
    case HIR_BLOCK:        return "BLOCK";
    case HIR_ASSIGN:       return "ASSIGN";
    case HIR_RETURN:       return "RETURN";
    case HIR_IF:           return "IF";
    case HIR_MATCHARM:     return "MATCHARM";
    case HIR_WHILE:        return "WHILE";
    case HIR_LOOP:         return "LOOP";
    case HIR_ILITERAL:     return "ILITERAL";
    case HIR_FLITERAL:     return "FLITERAL";
    case HIR_SLITERAL:     return "SLITERAL";
    case HIR_CLITERAL:     return "CLITERAL";
    case HIR_BINOP:        return "BINOP";
    case HIR_UNOP:         return "UNOP";
    case HIR_CAST:         return "CAST";
    case HIR_IDENT:        return "IDENT";
    case HIR_STRUCTLIT:    return "STRUCTLIT";
    case HIR_FIELDACCESS:  return "FIELDACCESS";
    case HIR_CALL:         return "CALL";
    case HIR_ARRAYLIT:     return "ARRAYLIT";
    case HIR_INDEX:        return "INDEX";
    default:               return "???";
    }
}

char *hir_node_print(hnode node_)
{
    char *buf = (char*)malloc(256);
    if (!buf)
        return NULL;

    switch (node_.kind) {
    case HIR_FUNCDECL:
        snprintf(buf, 256, "%s '%s' -> %s  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.sv ? node_.sv : "(null)",
                 (node_.type ? node_.type->name : "?"),
                 node_.child, node_.next);
        break;
    case HIR_VARDECL:
        snprintf(buf, 256, "%s '%s': %s  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.sv ? node_.sv : "(null)",
                 (node_.type ? node_.type->name : "?"),
                 node_.child, node_.next);
        break;
    case HIR_IDENT:
        snprintf(buf, 256, "%s -> #%lld: %s  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 (long long)node_.iv,
                 (node_.type ? node_.type->name : "?"),
                 node_.child, node_.next);
        break;
    case HIR_ILITERAL:
        snprintf(buf, 256, "%s %lld: %s  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 (long long)node_.iv,
                 (node_.type ? node_.type->name : "?"),
                 node_.child, node_.next);
        break;
    case HIR_FLITERAL:
        snprintf(buf, 256, "%s %f: %s  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.fv,
                 (node_.type ? node_.type->name : "?"),
                 node_.child, node_.next);
        break;
    case HIR_SLITERAL:
        snprintf(buf, 256, "%s \"%s\"  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.sv ? node_.sv : "(null)",
                 node_.child, node_.next);
        break;
    case HIR_CLITERAL:
        snprintf(buf, 256, "%s '%c'  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.cv,
                 node_.child, node_.next);
        break;
    case HIR_BINOP:
        snprintf(buf, 256, "%s '%s': %s  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 binop_symbol(node_.op),
                 (node_.type ? node_.type->name : "?"),
                 node_.child, node_.next);
        break;
    case HIR_UNOP:
        snprintf(buf, 256, "%s '%s': %s  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 binop_symbol(node_.op),
                 (node_.type ? node_.type->name : "?"),
                 node_.child, node_.next);
        break;
    case HIR_CAST:
        snprintf(buf, 256, "%s -> %s  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 (node_.type ? node_.type->name : "?"),
                 node_.child, node_.next);
        break;
    case HIR_STRUCTLIT:
        snprintf(buf, 256, "%s %s  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 (node_.type && node_.type->name ? node_.type->name : "?"),
                 node_.child, node_.next);
        break;
    case HIR_FIELDACCESS:
        snprintf(buf, 256, "%s .%s: %s  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.sv ? node_.sv : "?",
                 (node_.type ? node_.type->name : "?"),
                 node_.child, node_.next);
        break;
    case HIR_CALL:
        snprintf(buf, 256, "%s @%s -> %s  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.sv ? node_.sv : "?",
                 (node_.type ? node_.type->name : "?"),
                 node_.child, node_.next);
        break;
    default:
        snprintf(buf, 256, "%s: %s  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 (node_.type ? node_.type->name : "?"),
                 node_.child, node_.next);
        break;
    }

    return buf;
}

static void node_label(const hnode *n, char *buf, size_t buf_sz)
{
    switch (n->kind) {
    case HIR_FUNCDECL:
        snprintf(buf, buf_sz, "%s '%s' -> %s",
                 kind_name(n->kind),
                 n->sv ? n->sv : "?",
                 (n->type ? n->type->name : "?"));
        break;
    case HIR_VARDECL:
        snprintf(buf, buf_sz, "%s '%s': %s",
                 kind_name(n->kind),
                 n->sv ? n->sv : "?",
                 (n->type ? n->type->name : "?"));
        break;
    case HIR_IDENT:
        snprintf(buf, buf_sz, "%s -> #%lld: %s",
                 kind_name(n->kind),
                 (long long)n->iv,
                 (n->type ? n->type->name : "?"));
        break;
    case HIR_ILITERAL:
        snprintf(buf, buf_sz, "%s %lld: %s",
                 kind_name(n->kind),
                 (long long)n->iv,
                 (n->type ? n->type->name : "?"));
        break;
    case HIR_FLITERAL:
        snprintf(buf, buf_sz, "%s %f: %s",
                 kind_name(n->kind),
                 n->fv,
                 (n->type ? n->type->name : "?"));
        break;
    case HIR_SLITERAL:
        snprintf(buf, buf_sz, "%s \"%s\"",
                 kind_name(n->kind),
                 n->sv ? n->sv : "");
        break;
    case HIR_CLITERAL:
        snprintf(buf, buf_sz, "%s '%c'",
                 kind_name(n->kind),
                 n->cv);
        break;
    case HIR_BINOP:
        snprintf(buf, buf_sz, "%s '%s': %s",
                 kind_name(n->kind),
                 binop_symbol(n->op),
                 (n->type ? n->type->name : "?"));
        break;
    case HIR_MATCHARM:
        snprintf(buf, buf_sz, "%s '%s': %s",
                 kind_name(n->kind),
                 binop_symbol(n->op),
                 (n->type ? n->type->name : "?"));
        break;
    case HIR_UNOP:
        snprintf(buf, buf_sz, "%s '%s': %s",
                 kind_name(n->kind),
                 binop_symbol(n->op),
                 (n->type ? n->type->name : "?"));
        break;
    case HIR_CAST:
        snprintf(buf, buf_sz, "%s -> %s",
                 kind_name(n->kind),
                 (n->type ? n->type->name : "?"));
        break;
    case HIR_STRUCTLIT:
        snprintf(buf, buf_sz, "%s %s",
                 kind_name(n->kind),
                 (n->type && n->type->name ? n->type->name : "?"));
        break;
    case HIR_FIELDACCESS:
        snprintf(buf, buf_sz, "%s .%s: %s",
                 kind_name(n->kind),
                 n->sv ? n->sv : "?",
                 (n->type ? n->type->name : "?"));
        break;
    case HIR_CALL:
        snprintf(buf, buf_sz, "%s @%s -> %s",
                 kind_name(n->kind),
                 n->sv ? n->sv : "?",
                 (n->type ? n->type->name : "?"));
        break;
    default:
        snprintf(buf, buf_sz, "%s: %s",
                 kind_name(n->kind),
                 (n->type ? n->type->name : "?"));
        break;
    }
}

static void print_children(strbuf *sb, hir_vector hir, int idx,
                           const char *indent)
{
    int cur = hir.data[idx].child;
    while (cur >= 0) {
        int next = hir.data[cur].next;
        bool is_last = (next < 0);

        char label[128];
        node_label(&hir.data[cur], label, sizeof(label));

        strbuf_addf(sb, "%s%s %s\n",
                    indent,
                    is_last ? "`--" : "|--",
                    label);

        if (hir.data[cur].child >= 0) {
            char child_indent[256];
            snprintf(child_indent, sizeof(child_indent), "%s%s",
                     indent,
                     is_last ? "    " : "|   ");
            print_children(sb, hir, cur, child_indent);
        }

        cur = next;
    }
}

char *hir_print_tree(hir_vector hir)
{
    strbuf sb;
    int i;
    bool *referenced;
    int root_cnt = 0;

    strbuf_init(&sb, 4096);

    if (hir.size == 0) {
        strbuf_add(&sb, "(empty)\n");
        return strbuf_detach(&sb);
    }

    referenced = malloc((size_t)hir.size * sizeof(bool));
    if (!referenced) abort();
    memset(referenced, 0, (size_t)hir.size * sizeof(bool));

    for (i = 0; i < hir.size; i++) {
        const hnode *n = &hir.data[i];
        if (n->child >= 0) referenced[n->child] = true;
        if (n->next >= 0)  referenced[n->next]  = true;
    }

    for (i = 0; i < hir.size; i++) {
        if (referenced[i]) continue;

        char label[128];
        node_label(&hir.data[i], label, sizeof(label));
        strbuf_addf(&sb, "%s\n", label);

        if (hir.data[i].child >= 0)
            print_children(&sb, hir, i, "");

        root_cnt++;
    }

    free(referenced);

    if (root_cnt == 0)
        strbuf_add(&sb, "(no roots)\n");

    return strbuf_detach(&sb);
}
