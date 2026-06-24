/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * hir.h — HIR node kind enum and the hnode struct.
 *
 * HIR sits between sema and codegen.  Every node carries a resolved Type*;
 * identifier references point to their declaration node by index.
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
#ifndef HIR_HIR_H
#define HIR_HIR_H

#include "collect/vector.h"
#include "token/token.h"
#include "type/type.h"

typedef enum {
    HIR_NONE,

    HIR_FUNCDECL,
    HIR_VARDECL,

    HIR_BLOCK,
    HIR_ASSIGN,
    HIR_RETURN,
    HIR_IF,
    HIR_MATCHARM,
    HIR_WHILE,
    HIR_LOOP,

    HIR_ILITERAL,
    HIR_FLITERAL,
    HIR_SLITERAL,
    HIR_CLITERAL,

    HIR_BINOP,
    HIR_UNOP,
    HIR_CAST,
    HIR_IDENT,

    HIR_STRUCTLIT,
    HIR_FIELDACCESS,

    HIR_COUNT,
} hkind;

/*
 * Flat HIR node.  child/next are indices into hir_vector (-1 = none).
 * Union fields by kind: FUNCDECL/VARDECL/SLITERAL use .sv, ILITERAL/(IDENT?) use .iv,
 * FLITERAL uses .fv, CLITERAL uses .cv, BINOP/UNOP use .op.
 */
typedef struct {
    hkind kind;
    const Type *type;
    int child;
    int next;
    union {
        int64_t iv;
        double fv;
        char *sv;
        char cv;
        tktype op;
    };
} hnode;

DECLARE_VECTOR(hnode, hir)

char *hir_node_print(hnode node_);
char *hir_print_tree(hir_vector hir);

#endif /* HIR_HIR_H */
