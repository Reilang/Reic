/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * codegen.c — LLVM IR text emitter: helpers, function orchestration, entry.
 *
 * codegen_emit() walks the HIR and writes a .ll file.
 * Variables use alloca-in-entry strategy (no phi nodes needed).
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
#include "codegen/codegen_internal.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Returns a pointer to a static buffer.  Callers must copy the result before
 * calling any function that may indirectly call buf_alloc() again (e.g. nested
 * emit_expr / emit_stmt).  Existing call sites already copy via snprintf into
 * local buffers immediately after each emit_expr/emit_stmt call. */
char *buf_alloc(void)
{
    static char buf[64];
    return buf;
}

const char *llvm_ty(const Type *type)
{
    return type_llvm_name(type);
}

const char *icmp_cond(tktype op, bool is_signed)
{
    switch (op) {
    case TK_EQUAL:         return "eq";
    case TK_NOTEQUAL:      return "ne";
    case TK_OABRACKET:     return is_signed ? "slt" : "ult";
    case TK_CABRACKET:     return is_signed ? "sgt" : "ugt";
    case TK_LESSEQUAL:     return is_signed ? "sle" : "ule";
    case TK_GREATEREQUAL:  return is_signed ? "sge" : "uge";
    default:               return "eq";
    }
}

const char *fcmp_cond(tktype op)
{
    switch (op) {
    case TK_EQUAL:         return "oeq";
    case TK_NOTEQUAL:      return "one";
    case TK_OABRACKET:     return "olt";
    case TK_CABRACKET:     return "ogt";
    case TK_LESSEQUAL:     return "ole";
    case TK_GREATEREQUAL:  return "oge";
    default:               return "oeq";
    }
}

const char *cast_op(const Type *src, const Type *dst)
{
    int sw = type_width(src);
    int dw = type_width(dst);

    if (type_is_float(src) && type_is_float(dst)) {
        if (sw < dw) return "fpext";
        if (sw > dw) return "fptrunc";
        return "bitcast";
    }
    if (type_is_integer(src) && type_is_float(dst))
        return type_is_signed(src) ? "sitofp" : "uitofp";
    if (type_is_float(src) && type_is_integer(dst))
        return type_is_signed(dst) ? "fptosi" : "fptoui";

    /* Integer <-> Integer */
    if (sw < dw)
        return type_is_signed(src) ? "sext" : "zext";
    if (sw > dw)
        return "trunc";
    return "bitcast";
}

const char *cg_new_reg(CgCtx *ctx)
{
    char *b = buf_alloc();
    snprintf(b, 64, "%%.%d", ctx->reg_cnt++);
    return b;
}

const char *cg_new_label(CgCtx *ctx, const char *prefix)
{
    char *b = buf_alloc();
    snprintf(b, 64, "%s_%d", prefix, ctx->lbl_cnt++);
    return b;
}

void collect_vardecls(CgCtx *ctx, int idx)
{
    if (idx < 0) return;
    hnode *n = &ctx->hir->data[idx];
    if (n->kind == HIR_VARDECL && ctx->alloca_map[idx] == NULL) {
        ctx->alloca_map[idx] = malloc(16);
        snprintf(ctx->alloca_map[idx], 16, "%%.%d", ctx->reg_cnt++);
    }
    int cur = n->child;
    while (cur >= 0) {
        collect_vardecls(ctx, cur);
        cur = ctx->hir->data[cur].next;
    }
}

static void emit_func(CgCtx *ctx, int func_idx)
{
    hnode *fn = &ctx->hir->data[func_idx];

    ctx->reg_cnt = 0;
    ctx->lbl_cnt = 0;
    ctx->func_ret_type = fn->type;

    /*
     * HIR structure for FUNCDECL:
     *   .child -> VARDECL(param0) -> VARDECL(param1) -> HIR_BLOCK (body)
     * Params are the VARDECL nodes before HIR_BLOCK in the direct child chain.
     */
    int param_idx[64];
    int nparams = 0;
    int cur = fn->child;
    int body_idx = -1;
    while (cur >= 0 && ctx->hir->data[cur].kind != HIR_BLOCK) {
        if (ctx->hir->data[cur].kind == HIR_VARDECL)
            param_idx[nparams++] = cur;
        cur = ctx->hir->data[cur].next;
    }
    body_idx = cur; /* HIR_BLOCK or -1 */

    /* Collect all VARDECL */
    ctx->alloca_map = calloc((size_t)ctx->hir->size, sizeof(char *));
    cur = fn->child;
    while (cur >= 0) {
        collect_vardecls(ctx, cur);
        cur = ctx->hir->data[cur].next;
    }

    /* Emit function header. */
    const char *ret_ty = llvm_ty(fn->type);
    strbuf_addf(&ctx->sb, "define %s @%s(", ret_ty, fn->sv);
    for (int i = 0; i < nparams; i++) {
        hnode *p = &ctx->hir->data[param_idx[i]];
        if (i > 0) strbuf_addf(&ctx->sb, ", ");
        strbuf_addf(&ctx->sb, "%s %%%s", llvm_ty(p->type), p->sv);
    }
    strbuf_addf(&ctx->sb, ") {\nentry:\n");

    /* Emit allocas for all collected vars. */
    for (int i = 0; i < ctx->hir->size; i++) {
        if (ctx->alloca_map[i] == NULL) continue;
        hnode *v = &ctx->hir->data[i];
        strbuf_addf(&ctx->sb, "  %s = alloca %s\n",
                    ctx->alloca_map[i], llvm_ty(v->type));
    }

    /* Store param values into their allocas. */
    for (int i = 0; i < nparams; i++) {
        hnode *p = &ctx->hir->data[param_idx[i]];
        strbuf_addf(&ctx->sb, "  store %s %%%s, %s* %s\n",
                    llvm_ty(p->type), p->sv,
                    llvm_ty(p->type), ctx->alloca_map[param_idx[i]]);
    }

    /* Emit body (HIR_BLOCK). */
    if (body_idx >= 0)
        emit_stmt(ctx, body_idx);

    strbuf_addf(&ctx->sb, "}\n\n");

    /* Cleanup. */
    for (int i = 0; i < ctx->hir->size; i++)
        free(ctx->alloca_map[i]);
    free(ctx->alloca_map);
    ctx->alloca_map = NULL;
}

static void emit_struct_type(CgCtx *ctx, const Type *t)
{
    int i;

    if (!t || t->kind != TYPEK_STRUCT || !t->name) return;

    for (i = 0; i < ctx->emitted_count; i++) {
        if (ctx->emitted_types[i] == t) return;
    }

    /* Emit nested struct types first. */
    for (i = 0; i < t->field_count; i++) {
        if (t->field_types[i] && t->field_types[i]->kind == TYPEK_STRUCT)
            emit_struct_type(ctx, t->field_types[i]);
    }

    strbuf_addf(&ctx->sb, "%%%s = type { ", t->name);
    for (i = 0; i < t->field_count; i++) {
        if (i > 0) strbuf_addf(&ctx->sb, ", ");
        strbuf_addf(&ctx->sb, "%s", llvm_ty(t->field_types[i]));
    }
    strbuf_addf(&ctx->sb, " }\n");

    if (ctx->emitted_count < 64)
        ctx->emitted_types[ctx->emitted_count++] = t;
}

void emit_all_struct_types(CgCtx *ctx)
{
    int i;
    for (i = 0; i < ctx->hir->size; i++) {
        const Type *t = ctx->hir->data[i].type;
        if (t && t->kind == TYPEK_STRUCT)
            emit_struct_type(ctx, t);
    }
}

static void emit_program(CgCtx *ctx)
{
    /* gcc says it would be negative, make it happy */
    if (ctx->hir->size <= 0) return;

    /* Find root HIR_FUNCDECL nodes (not referenced as child/next by others). */
    bool *referenced = calloc((size_t)ctx->hir->size, sizeof(bool));

    for (int i = 0; i < ctx->hir->size; i++) {
        hnode *n = &ctx->hir->data[i];
        if (n->child >= 0) referenced[n->child] = true;
        if (n->next >= 0)  referenced[n->next]  = true;
    }

    for (int i = 0; i < ctx->hir->size; i++) {
        if (!referenced[i] && ctx->hir->data[i].kind == HIR_FUNCDECL)
            emit_func(ctx, i);
    }

    free(referenced);
}

int codegen_emit(hir_vector *hir, const char *output_path)
{
    CgCtx ctx;
    strbuf_init(&ctx.sb, 4096);
    ctx.hir = hir;
    ctx.lbl_cnt = 0;
    ctx.reg_cnt = 0;
    ctx.alloca_map = NULL;
    ctx.func_ret_type = TYPE_UNIT;
    ctx.emitted_count = 0;
    memset(ctx.emitted_types, 0, sizeof(ctx.emitted_types));

    emit_all_struct_types(&ctx);
    emit_program(&ctx);

    FILE *f = fopen(output_path, "w");
    if (!f) {
        fprintf(stderr, "codegen: cannot open '%s'\n", output_path);
        strbuf_free(&ctx.sb);
        return -1;
    }
    fwrite(ctx.sb.data, 1, ctx.sb.len, f);
    fclose(f);
    strbuf_free(&ctx.sb);
    return 0;
}
