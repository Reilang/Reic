/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * codegen_stmt.c — LLVM IR emission for statements and control flow.
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

bool can_use_switch(CgCtx *ctx, int if_idx, const Type *scr_type)
{
    hnode *if_n = &ctx->hir->data[if_idx];
    int scrutinee_idx = if_n->child;
    if (scrutinee_idx < 0) return false;

    /* LLVM switch only supports integer types. */
    if (type_is_float(scr_type)) return false;

    int arm = ctx->hir->data[scrutinee_idx].next;
    if (arm < 0) return false;

    while (arm >= 0) {
        hnode *a = &ctx->hir->data[arm];
        if (a->kind != HIR_MATCHARM) { arm = a->next; continue; }
        if (a->op != TK_EQUAL) return false;

        int pat_idx = a->child;
        if (pat_idx < 0) return false;
        hnode *pat = &ctx->hir->data[pat_idx];
        if (pat->kind != HIR_ILITERAL) return false;
        if (pat->type != scr_type) return false;

        arm = a->next;
    }
    return true;
}

void emit_if_switch(CgCtx *ctx, int if_idx, const char *scr_reg,
                     const Type *scr_type, const char *merge_label_arg)
{
    hnode *if_n = &ctx->hir->data[if_idx];
    int scrutinee_idx = if_n->child;
    const char *ty = llvm_ty(scr_type);

    char merge_label[64];
    snprintf(merge_label, sizeof(merge_label), "%s", merge_label_arg);

    int case_vals[64];
    char case_labels[64][64];
    int ncases = 0;

    int arm = ctx->hir->data[scrutinee_idx].next;
    while (arm >= 0) {
        hnode *a = &ctx->hir->data[arm];
        if (a->kind == HIR_MATCHARM) {
            int pat_idx = a->child;
            case_vals[ncases] = (int)ctx->hir->data[pat_idx].iv;
            snprintf(case_labels[ncases], sizeof(case_labels[ncases]),
                     "%s", cg_new_label(ctx, "if_arm"));
            ncases++;
        }
        arm = a->next;
    }

    /* Emit switch. */
    strbuf_addf(&ctx->sb, "  switch %s %s, label %%%s [\n", ty, scr_reg, merge_label);
    for (int i = 0; i < ncases; i++) {
        strbuf_addf(&ctx->sb, "    %s %d, label %%%s\n",
                    ty, case_vals[i], case_labels[i]);
    }
    strbuf_addf(&ctx->sb, "  ]\n");

    /* Emit arm bodies. */
    int case_i = 0;
    arm = ctx->hir->data[scrutinee_idx].next;
    while (arm >= 0) {
        hnode *a = &ctx->hir->data[arm];
        if (a->kind == HIR_MATCHARM) {
            strbuf_addf(&ctx->sb, "\n%s:\n", case_labels[case_i]);

            int pat_idx = a->child;
            int body = ctx->hir->data[pat_idx].next;
            while (body >= 0) {
                emit_stmt(ctx, body);
                body = ctx->hir->data[body].next;
            }

            strbuf_addf(&ctx->sb, "  br label %%%s\n", merge_label);
            case_i++;
        }
        arm = a->next;
    }
}

void emit_if_chain(CgCtx *ctx, int if_idx, const char *scr_reg,
                    const Type *scr_type, const char *merge_label_arg)
{
    hnode *if_n = &ctx->hir->data[if_idx];
    int scrutinee_idx = if_n->child;
    const char *ty = llvm_ty(scr_type);
    bool scr_float = type_is_float(scr_type);
    bool scr_signed = type_is_signed(scr_type);

    char merge_label[64];
    snprintf(merge_label, sizeof(merge_label), "%s", merge_label_arg);

    /* Collect arms. */
    int arm_list[64];
    int narms = 0;
    int cur = ctx->hir->data[scrutinee_idx].next;
    while (cur >= 0) {
        hnode *a = &ctx->hir->data[cur];
        if (a->kind == HIR_MATCHARM) {
            arm_list[narms++] = cur;
        }
        cur = a->next;
    }

    if (narms == 0) {
        strbuf_addf(&ctx->sb, "  br label %%%s\n", merge_label);
        return;
    }

    /* Emit check chain. */
    for (int i = 0; i < narms; i++) {
        int arm_idx = arm_list[i];
        hnode *arm_n = &ctx->hir->data[arm_idx];

        char arm_label[64], next_label[64];
        snprintf(arm_label, sizeof(arm_label), "%s",
                 cg_new_label(ctx, "if_arm"));
        if (i + 1 < narms)
            snprintf(next_label, sizeof(next_label), "%s", cg_new_label(ctx, "if_check"));
        else
            snprintf(next_label, sizeof(next_label), "%s", merge_label);

        int pat_idx = arm_n->child;
        char pat_buf[64];
        const char *pat_reg = emit_expr(ctx, pat_idx);
        snprintf(pat_buf, sizeof(pat_buf), "%s", pat_reg);

        const char *cond_reg = cg_new_reg(ctx);
        if (scr_float) {
            const char *cond = fcmp_cond(arm_n->op);
            strbuf_addf(&ctx->sb, "  %s = fcmp %s %s %s, %s\n",
                        cond_reg, cond, ty, scr_reg, pat_buf);
        } else {
            const char *cond = icmp_cond(arm_n->op, scr_signed);
            strbuf_addf(&ctx->sb, "  %s = icmp %s %s %s, %s\n",
                        cond_reg, cond, ty, scr_reg, pat_buf);
        }
        strbuf_addf(&ctx->sb, "  br i1 %s, label %%%s, label %%%s\n",
                    cond_reg, arm_label, next_label);

        /* Emit arm body. */
        strbuf_addf(&ctx->sb, "\n%s:\n", arm_label);
        int body = ctx->hir->data[pat_idx].next;
        while (body >= 0) {
            emit_stmt(ctx, body);
            body = ctx->hir->data[body].next;
        }
        strbuf_addf(&ctx->sb, "  br label %%%s\n", merge_label);

        /* Next check label (if not the last). */
        if (i + 1 < narms)
            strbuf_addf(&ctx->sb, "\n%s:\n", next_label);
    }
}

void emit_stmt(CgCtx *ctx, int idx)
{
    if (idx < 0) return;

    hnode *n = &ctx->hir->data[idx];

    switch (n->kind) {
    case HIR_BLOCK: {
        int cur = n->child;
        while (cur >= 0) {
            emit_stmt(ctx, cur);
            cur = ctx->hir->data[cur].next;
        }
        break;
    }
    case HIR_VARDECL:
        /* Alloca already emitted in entry block. Handle initializer store. */
        if (n->child >= 0) {
            const char *ptr = ctx->alloca_map[idx];
            char rhs_buf[64];
            const char *rhs = emit_expr(ctx, n->child);
            snprintf(rhs_buf, sizeof(rhs_buf), "%s", rhs);
            const char *ty = llvm_ty(n->type);
            strbuf_addf(&ctx->sb, "  store %s %s, %s* %s\n", ty, rhs_buf, ty, ptr);
        }
        break;

    case HIR_ASSIGN: {
        int target_idx = n->child;
        hnode *target = &ctx->hir->data[target_idx];
        int rhs_idx = target->next;
        char rhs_buf[64];
        const char *rhs = emit_expr(ctx, rhs_idx);
        snprintf(rhs_buf, sizeof(rhs_buf), "%s", rhs);

        if (target->kind == HIR_FIELDACCESS) {
            const char *ptr = emit_ptr(ctx, target_idx);
            const char *fty = llvm_ty(target->type);
            strbuf_addf(&ctx->sb, "  store %s %s, %s* %s\n",
                        fty, rhs_buf, fty, ptr);
        } else {
            int decl_idx = (int)target->iv;
            const char *ptr = ctx->alloca_map[decl_idx];
            const char *ty = llvm_ty(target->type);
            strbuf_addf(&ctx->sb, "  store %s %s, %s* %s\n",
                        ty, rhs_buf, ty, ptr);
        }
        break;
    }

    case HIR_RETURN:
        if (n->child < 0) {
            strbuf_addf(&ctx->sb, "  ret void\n");
        } else {
            char val_buf[64];
            const char *val = emit_expr(ctx, n->child);
            snprintf(val_buf, sizeof(val_buf), "%s", val);
            const Type *ret_ty = n->type ? n->type : ctx->func_ret_type;
            const char *ty = llvm_ty(ret_ty);
            strbuf_addf(&ctx->sb, "  ret %s %s\n", ty, val_buf);
        }
        break;

    case HIR_IF: {
        int scrutinee_idx = n->child;
        char scr_buf[64];
        const char *scr_reg = emit_expr(ctx, scrutinee_idx);
        snprintf(scr_buf, sizeof(scr_buf), "%s", scr_reg);

        const Type *scr_type = ctx->hir->data[scrutinee_idx].type;
        char merge_label[64];
        snprintf(merge_label, sizeof(merge_label), "%s", cg_new_label(ctx, "if_merge"));

        if (can_use_switch(ctx, idx, scr_type)) {
            emit_if_switch(ctx, idx, scr_buf, scr_type, merge_label);
        } else {
            emit_if_chain(ctx, idx, scr_buf, scr_type, merge_label);
        }

        strbuf_addf(&ctx->sb, "\n%s:\n", merge_label);
        break;
    }

    case HIR_WHILE: {
        int cond_idx = n->child;
        int body_idx = (cond_idx >= 0) ? ctx->hir->data[cond_idx].next : -1;

        char cond_label[64], body_label[64], exit_label[64];
        snprintf(cond_label, sizeof(cond_label), "%s", cg_new_label(ctx, "while_cond"));
        snprintf(body_label, sizeof(body_label), "%s", cg_new_label(ctx, "while_body"));
        snprintf(exit_label, sizeof(exit_label), "%s", cg_new_label(ctx, "while_exit"));

        strbuf_addf(&ctx->sb, "  br label %%%s\n", cond_label);

        strbuf_addf(&ctx->sb, "\n%s:\n", cond_label);
        char cond_buf[64];
        const char *cond_reg = emit_expr(ctx, cond_idx);
        snprintf(cond_buf, sizeof(cond_buf), "%s", cond_reg);

        const Type *cond_type = ctx->hir->data[cond_idx].type;
        if (cond_type == TYPE_BOOL) {
            strbuf_addf(&ctx->sb, "  br i1 %s, label %%%s, label %%%s\n",
                        cond_buf, body_label, exit_label);
        } else if (type_is_float(cond_type)) {
            const char *ty = llvm_ty(cond_type);
            const char *bool_reg = cg_new_reg(ctx);
            strbuf_addf(&ctx->sb, "  %s = fcmp une %s %s, 0.0\n",
                        bool_reg, ty, cond_buf);
            strbuf_addf(&ctx->sb, "  br i1 %s, label %%%s, label %%%s\n",
                        bool_reg, body_label, exit_label);
        } else {
            const char *ty = llvm_ty(cond_type);
            const char *bool_reg = cg_new_reg(ctx);
            strbuf_addf(&ctx->sb, "  %s = icmp ne %s %s, 0\n",
                        bool_reg, ty, cond_buf);
            strbuf_addf(&ctx->sb, "  br i1 %s, label %%%s, label %%%s\n",
                        bool_reg, body_label, exit_label);
        }

        strbuf_addf(&ctx->sb, "\n%s:\n", body_label);
        emit_stmt(ctx, body_idx);
        strbuf_addf(&ctx->sb, "  br label %%%s\n", cond_label);

        strbuf_addf(&ctx->sb, "\n%s:\n", exit_label);
        break;
    }

    case HIR_LOOP: {
        char body_label[64];
        snprintf(body_label, sizeof(body_label), "%s", cg_new_label(ctx, "loop_body"));

        strbuf_addf(&ctx->sb, "  br label %%%s\n", body_label);
        strbuf_addf(&ctx->sb, "\n%s:\n", body_label);
        emit_stmt(ctx, n->child);
        strbuf_addf(&ctx->sb, "  br label %%%s\n", body_label);
        break;
    }

    default:
        break;
    }
}
