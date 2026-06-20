/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * codegen.c — LLVM IR text emitter from HIR.
 *
 * Walks the HIR and writes human-readable LLVM IR to a .ll file.
 * Variables use alloca-in-entry strategy (no phi nodes needed).
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "codegen/codegen.h"
#include "type/type.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    FILE *f;
    hir_vector *hir;
    int lbl_cnt;
    int reg_cnt;
    /* Map from HIR declaration index to alloca register name (malloc'd string). */
    char **alloca_map;
    type_tag func_ret_type;
} CgCtx;

static char *buf_alloc(void)
{
    static char pool[8][64];
    static int idx = 0;
    char *b = pool[idx];
    idx = (idx + 1) & 7;
    return b;
}

static const char *llvm_ty(type_tag tag)
{
    return type_info_of(tag)->llvm_name;
}

static const char *icmp_cond(tktype op, bool is_signed)
{
    switch (op) {
    case TK_EQUAL:      return "eq";
    case TK_NOTEQUAL:   return "ne";
    case TK_OABRACKET:  return is_signed ? "slt" : "ult";
    case TK_CABRACKET:  return is_signed ? "sgt" : "ugt";
    case TK_LESSEQUAL:  return is_signed ? "sle" : "ule";
    case TK_GREATEREQUAL: return is_signed ? "sge" : "uge";
    default:            return "eq";
    }
}

static const char *cast_op(type_tag src, type_tag dst)
{
    const type_info *si = type_info_of(src);
    const type_info *di = type_info_of(dst);
    if (si->width < di->width)
        return si->is_signed ? "sext" : "zext";
    if (si->width > di->width)
        return "trunc";
    return "bitcast";
}

static const char *cg_new_reg(CgCtx *ctx)
{
    char *b = buf_alloc();
    snprintf(b, 64, "%%.%d", ctx->reg_cnt++);
    return b;
}

static const char *cg_new_label(CgCtx *ctx, const char *prefix)
{
    char *b = buf_alloc();
    snprintf(b, 64, "%s_%d", prefix, ctx->lbl_cnt++);
    return b;
}

static const char *emit_expr(CgCtx *ctx, int idx);
static void emit_stmt(CgCtx *ctx, int idx);
static void collect_vardecls(CgCtx *ctx, int idx);

static void collect_vardecls(CgCtx *ctx, int idx)
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

static const char *emit_expr(CgCtx *ctx, int idx)
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

static bool can_use_switch(CgCtx *ctx, int if_idx, type_tag scr_type)
{
    hnode *if_n = &ctx->hir->data[if_idx];
    int scrutinee_idx = if_n->child;
    if (scrutinee_idx < 0) return false;

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

static void emit_if_switch(CgCtx *ctx, int if_idx, const char *scr_reg,
                           type_tag scr_type, const char *merge_label_arg)
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
    fprintf(ctx->f, "  switch %s %s, label %%%s [\n", ty, scr_reg, merge_label);
    for (int i = 0; i < ncases; i++) {
        fprintf(ctx->f, "    %s %d, label %%%s\n",
                ty, case_vals[i], case_labels[i]);
    }
    fprintf(ctx->f, "  ]\n");

    /* Emit arm bodies. */
    int case_i = 0;
    arm = ctx->hir->data[scrutinee_idx].next;
    while (arm >= 0) {
        hnode *a = &ctx->hir->data[arm];
        if (a->kind == HIR_MATCHARM) {
            fprintf(ctx->f, "\n%s:\n", case_labels[case_i]);

            int pat_idx = a->child;
            int body = ctx->hir->data[pat_idx].next;
            while (body >= 0) {
                emit_stmt(ctx, body);
                body = ctx->hir->data[body].next;
            }

            fprintf(ctx->f, "  br label %%%s\n", merge_label);
            case_i++;
        }
        arm = a->next;
    }
}

static void emit_if_chain(CgCtx *ctx, int if_idx, const char *scr_reg,
                          type_tag scr_type, const char *merge_label_arg)
{
    hnode *if_n = &ctx->hir->data[if_idx];
    int scrutinee_idx = if_n->child;
    const char *ty = llvm_ty(scr_type);
    bool scr_signed = type_info_of(scr_type)->is_signed;

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
        fprintf(ctx->f, "  br label %%%s\n", merge_label);
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
        const char *cond = icmp_cond(arm_n->op, scr_signed);
        fprintf(ctx->f, "  %s = icmp %s %s %s, %s\n",
                cond_reg, cond, ty, scr_reg, pat_buf);
        fprintf(ctx->f, "  br i1 %s, label %%%s, label %%%s\n",
                cond_reg, arm_label, next_label);

        /* Emit arm body. */
        fprintf(ctx->f, "\n%s:\n", arm_label);
        int body = ctx->hir->data[pat_idx].next;
        while (body >= 0) {
            emit_stmt(ctx, body);
            body = ctx->hir->data[body].next;
        }
        fprintf(ctx->f, "  br label %%%s\n", merge_label);

        /* Next check label (if not the last). */
        if (i + 1 < narms)
            fprintf(ctx->f, "\n%s:\n", next_label);
    }
}

static void emit_stmt(CgCtx *ctx, int idx)
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
            fprintf(ctx->f, "  store %s %s, %s* %s\n", ty, rhs_buf, ty, ptr);
        }
        break;

    case HIR_ASSIGN: {
        int target_idx = n->child;
        int decl_idx = (int)ctx->hir->data[target_idx].iv;
        const char *ptr = ctx->alloca_map[decl_idx];

        int rhs_idx = ctx->hir->data[target_idx].next;
        char rhs_buf[64];
        const char *rhs = emit_expr(ctx, rhs_idx);
        snprintf(rhs_buf, sizeof(rhs_buf), "%s", rhs);

        const char *ty = llvm_ty(ctx->hir->data[target_idx].type);
        fprintf(ctx->f, "  store %s %s, %s* %s\n", ty, rhs_buf, ty, ptr);
        break;
    }

    case HIR_RETURN:
        if (n->child < 0) {
            fprintf(ctx->f, "  ret void\n");
        } else {
            char val_buf[64];
            const char *val = emit_expr(ctx, n->child);
            snprintf(val_buf, sizeof(val_buf), "%s", val);
            type_tag ret_ty = (n->type != TYPE_VOID) ? n->type
                                                     : ctx->func_ret_type;
            const char *ty = llvm_ty(ret_ty);
            fprintf(ctx->f, "  ret %s %s\n", ty, val_buf);
        }
        break;

    case HIR_IF: {
        int scrutinee_idx = n->child;
        char scr_buf[64];
        const char *scr_reg = emit_expr(ctx, scrutinee_idx);
        snprintf(scr_buf, sizeof(scr_buf), "%s", scr_reg);

        type_tag scr_type = ctx->hir->data[scrutinee_idx].type;
        char merge_label[64];
        snprintf(merge_label, sizeof(merge_label), "%s", cg_new_label(ctx, "if_merge"));

        if (can_use_switch(ctx, idx, scr_type)) {
            emit_if_switch(ctx, idx, scr_buf, scr_type, merge_label);
        } else {
            emit_if_chain(ctx, idx, scr_buf, scr_type, merge_label);
        }

        fprintf(ctx->f, "\n%s:\n", merge_label);
        break;
    }

    case HIR_WHILE: {
        int cond_idx = n->child;
        int body_idx = (cond_idx >= 0) ? ctx->hir->data[cond_idx].next : -1;

        char cond_label[64], body_label[64], exit_label[64];
        snprintf(cond_label, sizeof(cond_label), "%s", cg_new_label(ctx, "while_cond"));
        snprintf(body_label, sizeof(body_label), "%s", cg_new_label(ctx, "while_body"));
        snprintf(exit_label, sizeof(exit_label), "%s", cg_new_label(ctx, "while_exit"));

        fprintf(ctx->f, "  br label %%%s\n", cond_label);

        fprintf(ctx->f, "\n%s:\n", cond_label);
        char cond_buf[64];
        const char *cond_reg = emit_expr(ctx, cond_idx);
        snprintf(cond_buf, sizeof(cond_buf), "%s", cond_reg);

        type_tag cond_type = ctx->hir->data[cond_idx].type;
        const char *ty = llvm_ty(cond_type);
        const char *bool_reg = cg_new_reg(ctx);
        fprintf(ctx->f, "  %s = icmp ne %s %s, 0\n", bool_reg, ty, cond_buf);
        fprintf(ctx->f, "  br i1 %s, label %%%s, label %%%s\n",
                bool_reg, body_label, exit_label);

        fprintf(ctx->f, "\n%s:\n", body_label);
        emit_stmt(ctx, body_idx);
        fprintf(ctx->f, "  br label %%%s\n", cond_label);

        fprintf(ctx->f, "\n%s:\n", exit_label);
        break;
    }

    case HIR_LOOP: {
        char body_label[64];
        snprintf(body_label, sizeof(body_label), "%s", cg_new_label(ctx, "loop_body"));

        fprintf(ctx->f, "  br label %%%s\n", body_label);
        fprintf(ctx->f, "\n%s:\n", body_label);
        emit_stmt(ctx, n->child);
        fprintf(ctx->f, "  br label %%%s\n", body_label);
        break;
    }

    default:
        break;
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
    fprintf(ctx->f, "define %s @%s(", ret_ty, fn->sv);
    for (int i = 0; i < nparams; i++) {
        hnode *p = &ctx->hir->data[param_idx[i]];
        if (i > 0) fprintf(ctx->f, ", ");
        fprintf(ctx->f, "%s %%%s", llvm_ty(p->type), p->sv);
    }
    fprintf(ctx->f, ") {\nentry:\n");

    /* Emit allocas for all collected vars. */
    for (int i = 0; i < ctx->hir->size; i++) {
        if (ctx->alloca_map[i] == NULL) continue;
        hnode *v = &ctx->hir->data[i];
        fprintf(ctx->f, "  %s = alloca %s\n",
                ctx->alloca_map[i], llvm_ty(v->type));
    }

    /* Store param values into their allocas. */
    for (int i = 0; i < nparams; i++) {
        hnode *p = &ctx->hir->data[param_idx[i]];
        fprintf(ctx->f, "  store %s %%%s, %s* %s\n",
                llvm_ty(p->type), p->sv,
                llvm_ty(p->type), ctx->alloca_map[param_idx[i]]);
    }

    /* Emit body (HIR_BLOCK). */
    if (body_idx >= 0)
        emit_stmt(ctx, body_idx);

    fprintf(ctx->f, "}\n\n");

    /* Cleanup. */
    for (int i = 0; i < ctx->hir->size; i++)
        free(ctx->alloca_map[i]);
    free(ctx->alloca_map);
    ctx->alloca_map = NULL;
}

static void emit_program(CgCtx *ctx)
{
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
    FILE *f = fopen(output_path, "w");
    if (!f) {
        fprintf(stderr, "codegen: cannot open '%s'\n", output_path);
        return -1;
    }

    CgCtx ctx;
    ctx.f = f;
    ctx.hir = hir;
    ctx.lbl_cnt = 0;
    ctx.reg_cnt = 0;
    ctx.alloca_map = NULL;
    ctx.func_ret_type = TYPE_VOID;

    emit_program(&ctx);

    fclose(f);
    return 0;
}
