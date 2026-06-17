/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * ast.c — AST node utility: debug string conversion and ASCII tree printing.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "ast/ast.h"
#include "collect/strbuf.h"
#include "type/type.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *kind_name(anode_kind kind)
{
    switch (kind) {
    case ANODE_NONE:       return "NONE";
    case ANODE_IDENT:      return "IDENT";
    case ANODE_IDENT_FUNC: return "IDENT_FUNC";
    case ANODE_IDENT_VAR:  return "IDENT_VAR";
    case ANODE_IDENT_TYPE: return "IDENT_TYPE";
    case ANODE_ILITERAL:   return "ILITERAL";
    case ANODE_FLITERAL:   return "FLITERAL";
    case ANODE_SLITERAL:   return "SLITERAL";
    case ANODE_CLITERAL:   return "CLITERAL";
    case ANODE_BINOP:      return "BINOP";
    case ANODE_UNOP:       return "UNOP";
    case ANODE_ASSIGN:     return "ASSIGN";
    case ANODE_CALL:       return "CALL";
    case ANODE_INDEX:      return "INDEX";
    case ANODE_BLOCK:      return "BLOCK";
    case ANODE_IF:         return "IF";
    case ANODE_WHILE:      return "WHILE";
    case ANODE_FOR:        return "FOR";
    case ANODE_RETURN:     return "RETURN";
    case ANODE_MATCHARM:   return "MATCHARM";
    case ANODE_FUNCDECL:   return "FUNCDECL";
    case ANODE_VARDECL:    return "VARDECL";
    case ANODE_TYPEDECL:   return "TYPEDECL";
    default:               return "???";
    }
}

char *anode_print(anode node_)
{
    char *buf = (char*)malloc(256);
    if (!buf)
        return NULL;

    switch (node_.kind) {
    case ANODE_IDENT:
    case ANODE_IDENT_FUNC:
    case ANODE_IDENT_VAR:
        snprintf(buf, 256, "%s '%s'  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.sv ? node_.sv : "(null)",
                 node_.child, node_.next);
        break;
    case ANODE_IDENT_TYPE:
        snprintf(buf, 256, "%s '%s'  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 type_info_of((type_tag)node_.iv)->name,
                 node_.child, node_.next);
        break;
    case ANODE_ILITERAL:
        snprintf(buf, 256, "%s %lld  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 (long long)node_.iv,
                 node_.child, node_.next);
        break;
    case ANODE_FLITERAL:
        snprintf(buf, 256, "%s %f  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.fv,
                 node_.child, node_.next);
        break;
    case ANODE_SLITERAL:
        snprintf(buf, 256, "%s \"%s\"  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.sv ? node_.sv : "(null)",
                 node_.child, node_.next);
        break;
    case ANODE_CLITERAL:
        snprintf(buf, 256, "%s '%c'  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.cv,
                 node_.child, node_.next);
        break;
    default:
        snprintf(buf, 256, "%s  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.child, node_.next);
        break;
    }

    return buf;
}

static const char *binop_symbol(tktype op)
{
    switch (op) {
    case TK_ADD:   return "+";
    case TK_MINUS: return "-";
    case TK_STAR:  return "*";
    case TK_SLASH: return "/";
    default:       return "?";
    }
}

static void node_label(const anode *n, char *buf, size_t buf_sz)
{
    switch (n->kind) {
    case ANODE_IDENT:
    case ANODE_IDENT_FUNC:
    case ANODE_IDENT_VAR:
        snprintf(buf, buf_sz, "%s '%s'",
                 kind_name(n->kind),
                 n->sv ? n->sv : "?");
        break;
    case ANODE_IDENT_TYPE:
        snprintf(buf, buf_sz, "%s '%s'",
                 kind_name(n->kind),
                 type_info_of((type_tag)n->iv)->name);
        break;
    case ANODE_ILITERAL:
        snprintf(buf, buf_sz, "%s %lld",
                 kind_name(n->kind),
                 (long long)n->iv);
        break;
    case ANODE_FLITERAL:
        snprintf(buf, buf_sz, "%s %f",
                 kind_name(n->kind),
                 n->fv);
        break;
    case ANODE_SLITERAL:
        snprintf(buf, buf_sz, "%s \"%s\"",
                 kind_name(n->kind),
                 n->sv ? n->sv : "");
        break;
    case ANODE_CLITERAL:
        snprintf(buf, buf_sz, "%s '%c'",
                 kind_name(n->kind),
                 n->cv);
        break;
    case ANODE_BINOP:
        snprintf(buf, buf_sz, "%s '%s'",
                 kind_name(n->kind),
                 binop_symbol(n->op));
        break;
    default:
        snprintf(buf, buf_sz, "%s", kind_name(n->kind));
        break;
    }
}

static void print_children(strbuf *sb, node_vector nodes, int idx,
                           const char *indent)
{
    int cur = nodes.data[idx].child;
    while (cur >= 0) {
        int next = nodes.data[cur].next;
        bool is_last = (next < 0);

        char label[128];
        node_label(&nodes.data[cur], label, sizeof(label));

        strbuf_addf(sb, "%s%s %s\n",
                    indent,
                    is_last ? "`--" : "|--",
                    label);

        if (nodes.data[cur].child >= 0) {
            char child_indent[256];
            snprintf(child_indent, sizeof(child_indent), "%s%s",
                     indent,
                     is_last ? "    " : "|   ");
            print_children(sb, nodes, cur, child_indent);
        }

        cur = next;
    }
}

char *ast_print_tree(node_vector nodes)
{
    strbuf sb;
    int i;
    bool *referenced;
    int root_cnt = 0;

    strbuf_init(&sb, 4096);

    if (nodes.size == 0) {
        strbuf_add(&sb, "(empty)\n");
        return strbuf_detach(&sb);
    }

    referenced = malloc((size_t)nodes.size * sizeof(bool));
    if (!referenced) abort();
    memset(referenced, 0, (size_t)nodes.size * sizeof(bool));

    for (i = 0; i < nodes.size; i++) {
        const anode *n = &nodes.data[i];
        if (n->child >= 0) referenced[n->child] = true;
        if (n->next >= 0)  referenced[n->next]  = true;
    }

    for (i = 0; i < nodes.size; i++) {
        if (referenced[i]) continue;

        char label[128];
        node_label(&nodes.data[i], label, sizeof(label));
        strbuf_addf(&sb, "%s\n", label);

        if (nodes.data[i].child >= 0)
            print_children(&sb, nodes, i, "");

        root_cnt++;
    }

    free(referenced);

    if (root_cnt == 0)
        strbuf_add(&sb, "(no roots)\n");

    return strbuf_detach(&sb);
}
