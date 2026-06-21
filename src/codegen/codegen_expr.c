/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * codegen_expr.c — LLVM IR emission for expressions.
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

const char *emit_expr(CgCtx *ctx, int idx)
{
    if (idx < 0) {
        char *b = buf_alloc();
        snprintf(b, 64, "undef");
        return b;
    }

    hnode *n = &ctx->hir->data[idx];

    switch (n->kind) {
    case HIR_ILITERAL: {
        char *b = buf_alloc();
        snprintf(b, 64, "%lld", (long long)n->iv);
        return b;
    }
    case HIR_FLITERAL: {
        char *b = buf_alloc();
        snprintf(b, 64, "%f", n->fv);
        return b;
    }
    case HIR_CLITERAL: {
        char *b = buf_alloc();
        snprintf(b, 64, "%d", (int)n->cv);
        return b;
    }
    case HIR_IDENT: {
        int decl_idx = (int)n->iv;
        const char *ptr = ctx->alloca_map[decl_idx];
        const char *ty = llvm_ty(n->type);
        const char *reg = cg_new_reg(ctx);
        fprintf(ctx->f, "  %s = load %s, %s* %s\n", reg, ty, ty, ptr);
        return reg;
    }
    case HIR_BINOP: {
        int lhs_idx = n->child;
        int rhs_idx = (lhs_idx >= 0) ? ctx->hir->data[lhs_idx].next : -1;

        char lhs_buf[64], rhs_buf[64];
        const char *lhs_reg = emit_expr(ctx, lhs_idx);
        snprintf(lhs_buf, sizeof(lhs_buf), "%s", lhs_reg);
        const char *rhs_reg = emit_expr(ctx, rhs_idx);
        snprintf(rhs_buf, sizeof(rhs_buf), "%s", rhs_reg);

        const char *ty = llvm_ty(n->type);
        bool is_comp = false;
        const char *opname = NULL;
        switch (n->op) {
        case TK_ADD:   opname = "add";  break;
        case TK_MINUS: opname = "sub";  break;
        case TK_STAR:  opname = "mul";  break;
        case TK_SLASH:
            opname = type_info_of(n->type)->is_signed ? "sdiv" : "udiv";
            break;
        case TK_EQUAL:
        case TK_NOTEQUAL:
        case TK_OABRACKET:
        case TK_CABRACKET:
        case TK_LESSEQUAL:
        case TK_GREATEREQUAL:
            is_comp = true;
            break;
        default:       opname = "add";  break;
        }

        if (is_comp) {
            const char *cond = icmp_cond(n->op,
                                         type_info_of(n->type)->is_signed);
            const char *cmp_res = cg_new_reg(ctx);
            fprintf(ctx->f, "  %s = icmp %s %s %s, %s\n",
                    cmp_res, cond, ty, lhs_buf, rhs_buf);
            const char *res = cg_new_reg(ctx);
            fprintf(ctx->f, "  %s = zext i1 %s to %s\n", res, cmp_res, ty);
            return res;
        }

        const char *res = cg_new_reg(ctx);
        fprintf(ctx->f, "  %s = %s %s %s, %s\n",
                res, opname, ty, lhs_buf, rhs_buf);
        return res;
    }
    case HIR_UNOP: {
        const char *opnd = emit_expr(ctx, n->child);
        const char *ty = llvm_ty(n->type);
        const char *res = cg_new_reg(ctx);

        if (n->op == TK_MINUS) {
            fprintf(ctx->f, "  %s = sub %s 0, %s\n", res, ty, opnd);
        } else if (n->op == TK_NOT) {
            fprintf(ctx->f, "  %s = xor %s %s, -1\n", res, ty, opnd);
        } else {
            fprintf(ctx->f, "  %s = add %s %s, 0\n", res, ty, opnd);
        }
        return res;
    }
    case HIR_CAST: {
        const char *inner = emit_expr(ctx, n->child);
        type_tag inner_type = ctx->hir->data[n->child].type;
        const char *op = cast_op(inner_type, n->type);
        const char *src_ty = llvm_ty(inner_type);
        const char *dst_ty = llvm_ty(n->type);
        const char *res = cg_new_reg(ctx);
        fprintf(ctx->f, "  %s = %s %s %s to %s\n",
                res, op, src_ty, inner, dst_ty);
        return res;
    }
    default:
        break;
    }

    char *b = buf_alloc();
    snprintf(b, 64, "undef");
    return b;
}
