/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * sema_internal.h — Shared declarations and template instantiations for
 * sema translation units.
 *
 * NOT part of the public API.  Included only by sema translation units.
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
#ifndef SEMA_SEMA_INTERNAL_H
#define SEMA_SEMA_INTERNAL_H

#include "ast/ast.h"
#include "collect/hashset.h"
#include "collect/vector.h"
#include "diag/diag.h"
#include "sema/sema.h"
#include "type/type.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DECLARE_SET(sym_entry, sym)
DECLARE_VECTOR(sym_set, sym_set)

sym_set *cur_scope(sym_set_vector *stack);
int cur_depth(const sym_set_vector *stack);
void sym_set_copy(sym_set *dst, const sym_set *src);
void diag_fmt(diag_vector *diags, level lv, int line, int col,
              const char *fmt, ...);
void scope_enter(sym_set_vector *stack);
void scope_leave(sym_set_vector *stack, diag_vector *diags);

const Type *common_type(const Type *a, const Type *b);
bool assignable_to(const Type *dst, const Type *src);

void sema_vardecl(node_vector nodes, sym_set_vector *stack, int idx,
                   diag_vector *diags, sema_vector *annot);
void sema_constdecl(node_vector nodes, sym_set_vector *stack, int idx,
                     diag_vector *diags, sema_vector *annot);
void sema_structdef_reg(node_vector nodes, sym_set_vector *stack, int cd_idx,
                         diag_vector *diags, sema_vector *annot);
void sema_funcdecl_reg(node_vector nodes, sym_set_vector *stack, int fn_idx,
                        diag_vector *diags, sema_vector *annot);

const Type *sema_expr(node_vector nodes, sym_set_vector *stack, int idx,
                      diag_vector *diags, sema_vector *annot);

void sema_block(node_vector nodes, sym_set_vector *stack,
                 int block_idx, diag_vector *diags, sema_vector *annot);
void sema_stmt(node_vector nodes, sym_set_vector *stack, int idx,
                diag_vector *diags, sema_vector *annot);
void sema_assign(node_vector nodes, sym_set_vector *stack, int idx,
                  diag_vector *diags, sema_vector *annot);
void sema_if(node_vector nodes, sym_set_vector *stack, int idx,
              diag_vector *diags, sema_vector *annot);
void sema_while(node_vector nodes, sym_set_vector *stack, int idx,
                 diag_vector *diags, sema_vector *annot);
void sema_loop(node_vector nodes, sym_set_vector *stack, int idx,
                diag_vector *diags, sema_vector *annot);
void sema_return(node_vector nodes, sym_set_vector *stack, int idx,
                  diag_vector *diags, sema_vector *annot);

#endif /* SEMA_SEMA_INTERNAL_H */
