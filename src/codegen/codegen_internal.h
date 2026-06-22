/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * codegen_internal.h — Shared declarations for codegen translation units.
 *
 * NOT part of the public API.  Included only by codegen translation units.
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
#ifndef CODEGEN_CODEGEN_INTERNAL_H
#define CODEGEN_CODEGEN_INTERNAL_H

#include "codegen/codegen.h"
#include "hir/hir.h"
#include "type/type.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- codegen context ---- */

typedef struct {
    FILE *f;
    hir_vector *hir;
    int lbl_cnt;
    int reg_cnt;
    char **alloca_map;
    const Type *func_ret_type;
} CgCtx;

/* ---- helpers (codegen.c) ---- */

char *buf_alloc(void);
const char *llvm_ty(const Type *type);
const char *icmp_cond(tktype op, bool is_signed);
const char *cast_op(const Type *src, const Type *dst);
const char *cg_new_reg(CgCtx *ctx);
const char *cg_new_label(CgCtx *ctx, const char *prefix);
void collect_vardecls(CgCtx *ctx, int idx);

/* ---- expressions (codegen_expr.c) ---- */

const char *emit_expr(CgCtx *ctx, int idx);

/* ---- statements (codegen_stmt.c) ---- */

void emit_stmt(CgCtx *ctx, int idx);
bool can_use_switch(CgCtx *ctx, int if_idx, const Type *scr_type);
void emit_if_switch(CgCtx *ctx, int if_idx, const char *scr_reg,
                    const Type *scr_type, const char *merge_label_arg);
void emit_if_chain(CgCtx *ctx, int if_idx, const char *scr_reg,
                   const Type *scr_type, const char *merge_label_arg);

#endif /* CODEGEN_CODEGEN_INTERNAL_H */
