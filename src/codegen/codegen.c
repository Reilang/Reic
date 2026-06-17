/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * codegen.c — LLVM IR text emitter (minimal).
 *
 * Walks the flat AST and writes human-readable LLVM IR to a .ll file.
 * Only handles what the parser actually produces:
 *   FUNCDECL -> BLOCK -> RETURN
 *
 * Output is compiled with:  clang output.ll -o prog.exe
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "codegen/codegen.h"
#include "type/type.h"

#include <stdio.h>
#include <string.h>

static int find_rettype(node_vector *nodes, int func_idx)
{
    int cur = nodes->data[nodes->data[func_idx].child].next;
    while (cur >= 0) {
        int nxt = nodes->data[cur].next;
        if (nxt >= 0 && nodes->data[nxt].kind == ANODE_BLOCK)
            return cur;
        cur = nxt;
    }
    return -1;
}

static int find_block(node_vector *nodes, int func_idx)
{
    int rt = find_rettype(nodes, func_idx);
    if (rt < 0) return -1;
    return nodes->data[rt].next;
}

static void emit_stmt(FILE *f, node_vector *nodes, int idx,
                      const char *ret_llvm);

static void emit_stmt_chain(FILE *f, node_vector *nodes, int idx,
                            const char *ret_llvm)
{
    while (idx >= 0) {
        emit_stmt(f, nodes, idx, ret_llvm);
        idx = nodes->data[idx].next;
    }
}

static void emit_stmt(FILE *f, node_vector *nodes, int idx,
                      const char *ret_llvm)
{
    if (idx < 0) return;

    anode *n = &nodes->data[idx];

    switch (n->kind) {
    case ANODE_BLOCK:
        emit_stmt_chain(f, nodes, n->child, ret_llvm);
        break;

    case ANODE_RETURN:
        if (n->child < 0) {
            if (strcmp(ret_llvm, "void") == 0)
                fprintf(f, "  ret void\n");
            else
                fprintf(f, "  ret %s undef\n", ret_llvm);
        }
        break;

    default:
        break;
    }
}

static void emit_params(FILE *f, node_vector *nodes, int func_idx)
{
    int cur = nodes->data[nodes->data[func_idx].child].next;
    int first = 1;

    while (cur >= 0 && nodes->data[cur].kind == ANODE_VARDECL) {
        anode *vd = &nodes->data[cur];
        int var_idx = vd->child;
        int type_idx = nodes->data[var_idx].next;
        const char *pname = nodes->data[var_idx].sv;
        type_tag ptag = (type_tag)nodes->data[type_idx].iv;
        const char *llvm_ty = type_info_of(ptag)->llvm_name;

        if (!first) fprintf(f, ", ");
        fprintf(f, "%s %%%s", llvm_ty, pname);
        first = 0;
        cur = vd->next;
    }
}

int codegen_emit(node_vector *nodes, const char *output_path)
{
    FILE *f = fopen(output_path, "w");
    int i;

    if (!f) {
        fprintf(stderr, "codegen: cannot open '%s'\n", output_path);
        return -1;
    }

    for (i = 0; i < nodes->size; i++) {
        anode *n = &nodes->data[i];

        if (n->kind != ANODE_FUNCDECL)
            continue;

        int name_idx    = n->child;
        int rettype_idx = find_rettype(nodes, i);
        int block_idx   = find_block(nodes, i);

        const char *func_name = (name_idx >= 0)
            ? nodes->data[name_idx].sv : "???";
        type_tag ret_tag = (rettype_idx >= 0)
            ? (type_tag)nodes->data[rettype_idx].iv : TYPE_VOID;
        const char *ret_llvm = type_info_of(ret_tag)->llvm_name;

        fprintf(f, "define %s @%s(", ret_llvm, func_name);
        emit_params(f, nodes, i);
        fprintf(f, ") {\n");
        fprintf(f, "entry:\n");

        if (block_idx >= 0)
            emit_stmt_chain(f, nodes, nodes->data[block_idx].child, ret_llvm);

        fprintf(f, "}\n\n");
    }

    fclose(f);
    return 0;
}
