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

const char *emit_ptr(CgCtx *ctx, int idx)
{
    hnode *n;

    if (idx < 0) return NULL;
    n = &ctx->hir->data[idx];

    switch (n->kind) {
    case HIR_IDENT: {
        int decl_idx = (int)n->iv;
        return ctx->alloca_map[decl_idx];
    }
    case HIR_FIELDACCESS: {
        char base_buf[64], sty_buf[64];
        const char *base = emit_ptr(ctx, n->child);
        const Type *stype;
        int fi, i;

        if (!base) return NULL;
        snprintf(base_buf, sizeof(base_buf), "%s", base);

        stype = ctx->hir->data[n->child].type;
        if (!stype || stype->kind != TYPEK_STRUCT) return NULL;

        fi = -1;
        for (i = 0; i < stype->field_count; i++) {
            if (strcmp(stype->field_names[i], n->sv) == 0) {
                fi = i;
                break;
            }
        }
        if (fi < 0) return NULL;

        {
            const char *sty = llvm_ty(stype);
            const char *gep = cg_new_reg(ctx);
            snprintf(sty_buf, sizeof(sty_buf), "%s", sty);
            strbuf_addf(&ctx->sb,
                        "  %s = getelementptr %s, %s* %s, i32 0, i32 %d\n",
                        gep, sty_buf, sty_buf, base_buf, fi);
            return gep;
        }
    }
    case HIR_INDEX: {
        int arr_idx = n->child;
        int idx_idx = (arr_idx >= 0) ? ctx->hir->data[arr_idx].next : -1;
        const Type *arr_ty = ctx->hir->data[arr_idx].type;
        char arr_ty_buf[64];
        char arr_ptr_buf[64];

        snprintf(arr_ty_buf, sizeof(arr_ty_buf), "%s", llvm_ty(arr_ty));

        const char *base_ptr = emit_ptr(ctx, arr_idx);
        if (!base_ptr) {
            const char *arr_val = emit_expr(ctx, arr_idx);
            const char *tmp = cg_new_reg(ctx);
            char tmp_buf[64];
            snprintf(tmp_buf, sizeof(tmp_buf), "%s", tmp);
            strbuf_addf(&ctx->sb, "  %s = alloca %s\n", tmp_buf, arr_ty_buf);
            strbuf_addf(&ctx->sb, "  store %s %s, %s* %s\n",
                        arr_ty_buf, arr_val, arr_ty_buf, tmp_buf);
            base_ptr = tmp;
        }
        snprintf(arr_ptr_buf, sizeof(arr_ptr_buf), "%s", base_ptr);

        {
            char idx_buf[64];
            const char *idx_val = emit_expr(ctx, idx_idx);
            snprintf(idx_buf, sizeof(idx_buf), "%s", idx_val);

            const char *gep = cg_new_reg(ctx);
            strbuf_addf(&ctx->sb,
                        "  %s = getelementptr %s, %s* %s, i32 0, i32 %s\n",
                        gep, arr_ty_buf, arr_ty_buf, arr_ptr_buf, idx_buf);
            return gep;
        }
    }
    default:
        return NULL;
    }
}

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
            if (!type_is_float(n->type))
                strbuf_addf(&ctx->sb, "  %s = xor %s %s, -1\n", res, ty, opnd);
            else
                strbuf_addf(&ctx->sb, "  %s = add %s %s, 0\n", res, ty, opnd);
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
    case HIR_STRUCTLIT: {
        char sty_buf[64], tmp_buf[64];
        const char *sty = llvm_ty(n->type);
        const char *tmp_ptr = cg_new_reg(ctx);
        int fc, fi;

        snprintf(sty_buf, sizeof(sty_buf), "%s", sty);
        snprintf(tmp_buf, sizeof(tmp_buf), "%s", tmp_ptr);
        strbuf_addf(&ctx->sb, "  %s = alloca %s\n", tmp_buf, sty_buf);

        fc = n->child;
        fi = 0;
        while (fc >= 0) {
            char fval_buf[64], fty_buf[64];
            const char *fval = emit_expr(ctx, fc);
            const char *fty = llvm_ty(ctx->hir->data[fc].type);
            snprintf(fval_buf, sizeof(fval_buf), "%s", fval);
            snprintf(fty_buf, sizeof(fty_buf), "%s", fty);

            {
                const char *gep = cg_new_reg(ctx);
                strbuf_addf(&ctx->sb,
                            "  %s = getelementptr %s, %s* %s, i32 0, i32 %d\n",
                            gep, sty_buf, sty_buf, tmp_buf, fi);
                strbuf_addf(&ctx->sb, "  store %s %s, %s* %s\n",
                            fty_buf, fval_buf, fty_buf, gep);
            }

            fc = ctx->hir->data[fc].next;
            fi++;
        }

        {
            const char *reg = cg_new_reg(ctx);
            strbuf_addf(&ctx->sb, "  %s = load %s, %s* %s\n",
                        reg, sty_buf, sty_buf, tmp_buf);
            return reg;
        }
    }
    case HIR_ARRAYLIT: {
        char sty_buf[64], tmp_buf[64];
        const char *sty = llvm_ty(n->type);
        const char *tmp_ptr = cg_new_reg(ctx);
        int cur, fi;

        snprintf(sty_buf, sizeof(sty_buf), "%s", sty);
        snprintf(tmp_buf, sizeof(tmp_buf), "%s", tmp_ptr);
        strbuf_addf(&ctx->sb, "  %s = alloca %s\n", tmp_buf, sty_buf);

        cur = n->child;
        fi = 0;
        while (cur >= 0) {
            char fval_buf[64], fty_buf[64];
            const char *fval = emit_expr(ctx, cur);
            const char *fty = llvm_ty(ctx->hir->data[cur].type);
            snprintf(fval_buf, sizeof(fval_buf), "%s", fval);
            snprintf(fty_buf, sizeof(fty_buf), "%s", fty);

            {
                const char *gep = cg_new_reg(ctx);
                strbuf_addf(&ctx->sb,
                            "  %s = getelementptr %s, %s* %s, i32 0, i32 %d\n",
                            gep, sty_buf, sty_buf, tmp_buf, fi);
                strbuf_addf(&ctx->sb, "  store %s %s, %s* %s\n",
                            fty_buf, fval_buf, fty_buf, gep);
            }

            cur = ctx->hir->data[cur].next;
            fi++;
        }

        {
            const char *reg = cg_new_reg(ctx);
            strbuf_addf(&ctx->sb, "  %s = load %s, %s* %s\n",
                        reg, sty_buf, sty_buf, tmp_buf);
            return reg;
        }
    }
    case HIR_INDEX: {
        char ptr_buf[64], fty_buf[64];
        const char *ptr = emit_ptr(ctx, idx);
        const char *fty = llvm_ty(n->type);

        if (!ptr) {
            char *b = buf_alloc();
            snprintf(b, 64, "undef");
            return b;
        }
        snprintf(ptr_buf, sizeof(ptr_buf), "%s", ptr);
        snprintf(fty_buf, sizeof(fty_buf), "%s", fty);

        {
            const char *reg = cg_new_reg(ctx);
            strbuf_addf(&ctx->sb, "  %s = load %s, %s* %s\n",
                        reg, fty_buf, fty_buf, ptr_buf);
            return reg;
        }
    }
    case HIR_FIELDACCESS: {
        char ptr_buf[64], fty_buf[64];
        const char *ptr = emit_ptr(ctx, idx);
        const char *fty = llvm_ty(n->type);

        if (!ptr) {
            char *b = buf_alloc();
            snprintf(b, 64, "undef");
            return b;
        }
        snprintf(ptr_buf, sizeof(ptr_buf), "%s", ptr);
        snprintf(fty_buf, sizeof(fty_buf), "%s", fty);

        {
            const char *reg = cg_new_reg(ctx);
            strbuf_addf(&ctx->sb, "  %s = load %s, %s* %s\n",
                        reg, fty_buf, fty_buf, ptr_buf);
            return reg;
        }
    }
    case HIR_CALL: {
        char arg_regs[64][64];
        const char *arg_tys[64];
        int nargs = 0;
        int arg = n->child;

        while (arg >= 0 && nargs < 64) {
            const char *r = emit_expr(ctx, arg);
            snprintf(arg_regs[nargs], sizeof(arg_regs[0]), "%s", r);
            arg_tys[nargs] = llvm_ty(ctx->hir->data[arg].type);
            nargs++;
            arg = ctx->hir->data[arg].next;
        }

        const char *ret_ty = llvm_ty(n->type);
        const char *res = cg_new_reg(ctx);
        strbuf_addf(&ctx->sb, "  %s = call %s @%s(", res, ret_ty, n->sv);
        for (int i = 0; i < nargs; i++) {
            if (i > 0) strbuf_addf(&ctx->sb, ", ");
            strbuf_addf(&ctx->sb, "%s %s", arg_tys[i], arg_regs[i]);
        }
        strbuf_addf(&ctx->sb, ")\n");
        return res;
    }
    default:
        break;
    }

    char *b = buf_alloc();
    snprintf(b, 64, "undef");
    return b;
}
