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
        strbuf_addf(&ctx->sb, "  %s = load %s, %s* %s\n", reg, ty, ty, ptr);
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

        const Type *op_ty = (lhs_idx >= 0)
            ? ctx->hir->data[lhs_idx].type : n->type;
        const char *op_llvm_ty = llvm_ty(op_ty);
        bool is_float = type_is_float(op_ty);
        bool is_comp = false;
        const char *opname = NULL;
        switch (n->op) {
        case TK_ADD:   opname = is_float ? "fadd"  : "add";  break;
        case TK_MINUS: opname = is_float ? "fsub"  : "sub";  break;
        case TK_STAR:  opname = is_float ? "fmul"  : "mul";  break;
        case TK_SLASH:
            opname = is_float ? "fdiv"
                     : type_is_signed(n->type) ? "sdiv" : "udiv";
            break;
        case TK_EQUAL:
        case TK_NOTEQUAL:
        case TK_OABRACKET:
        case TK_CABRACKET:
        case TK_LESSEQUAL:
        case TK_GREATEREQUAL:
            is_comp = true;
            break;
        default:       opname = is_float ? "fadd" : "add";  break;
        }

        if (is_comp) {
            const char *cmp_res = cg_new_reg(ctx);
            if (is_float) {
                const char *cond = fcmp_cond(n->op);
                strbuf_addf(&ctx->sb, "  %s = fcmp %s %s %s, %s\n",
                            cmp_res, cond, op_llvm_ty, lhs_buf, rhs_buf);
            } else {
                const char *cond = icmp_cond(n->op,
                                             type_is_signed(op_ty));
                strbuf_addf(&ctx->sb, "  %s = icmp %s %s %s, %s\n",
                            cmp_res, cond, op_llvm_ty, lhs_buf, rhs_buf);
            }
            return cmp_res;
        }

        const char *res = cg_new_reg(ctx);
        const char *res_ty = llvm_ty(n->type);
        strbuf_addf(&ctx->sb, "  %s = %s %s %s, %s\n",
                    res, opname, res_ty, lhs_buf, rhs_buf);
        return res;
    }
    case HIR_UNOP: {
        const char *opnd = emit_expr(ctx, n->child);
        const char *ty = llvm_ty(n->type);
        const char *res = cg_new_reg(ctx);

        if (n->op == TK_MINUS) {
            if (type_is_float(n->type))
                strbuf_addf(&ctx->sb, "  %s = fneg %s %s\n", res, ty, opnd);
            else
                strbuf_addf(&ctx->sb, "  %s = sub %s 0, %s\n", res, ty, opnd);
        } else if (n->op == TK_NOT) {
            strbuf_addf(&ctx->sb, "  %s = xor %s %s, -1\n", res, ty, opnd);
        } else {
            strbuf_addf(&ctx->sb, "  %s = add %s %s, 0\n", res, ty, opnd);
        }
        return res;
    }
    case HIR_CAST: {
        const char *inner = emit_expr(ctx, n->child);
        const Type *inner_type = ctx->hir->data[n->child].type;
        const char *op = cast_op(inner_type, n->type);
        const char *src_ty = llvm_ty(inner_type);
        const char *dst_ty = llvm_ty(n->type);
        const char *res = cg_new_reg(ctx);
        strbuf_addf(&ctx->sb, "  %s = %s %s %s to %s\n",
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
