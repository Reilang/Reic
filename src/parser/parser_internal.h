/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * parser_internal.h — Shared declarations for parser sub-files.
 *
 * NOT part of the public API.  Included only by parser translation units so that
 * helper functions and parse_* functions can see each other across translation
 * units without going through parser.h.
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
#ifndef PARSER_PARSER_INTERNAL_H
#define PARSER_PARSER_INTERNAL_H

#include "ast/ast.h"
#include "diag/diag.h"
#include "token/token.h"

/* ---- helpers (parser.c) ---- */

token curtok(const parser *p);
int at_eof(const parser *p);
void skip_newlines(parser *p);
int new_node(node_vector *nodes, anode_kind kind);
int new_ident(node_vector *nodes, const char *name, anode_kind kind);
void sync(parser *p);
int parse_block_body(parser *p, node_vector *nodes, diag_vector *diags,
                     const char *context);

/* ---- types (parser_decl.c) ---- */

int parse_type(parser *p, node_vector *nodes, diag_vector *diags, bool strict);

/* ---- declarations (parser_decl.c) ---- */

int parse_funcdef(parser *p, node_vector *nodes, diag_vector *diags);
int parse_vardecl(parser *p, node_vector *nodes, diag_vector *diags);
int parse_constdecl(parser *p, node_vector *nodes, diag_vector *diags);

/* ---- statements (parser_stmt.c) ---- */

int parse_stmt(parser *p, node_vector *nodes, diag_vector *diags);
int parse_if(parser *p, node_vector *nodes, diag_vector *diags);
int parse_while(parser *p, node_vector *nodes, diag_vector *diags);
int parse_loop(parser *p, node_vector *nodes, diag_vector *diags);
int parse_return(parser *p, node_vector *nodes, diag_vector *diags);

/* ---- expressions (parser_expr.c) ---- */

int parse_expr(parser *p, node_vector *nodes, diag_vector *diags, int min_prec);
int parse_primary(parser *p, node_vector *nodes, diag_vector *diags);
int parse_call(parser *p, node_vector *nodes, diag_vector *diags, int callee_idx);
int parse_assign(parser *p, node_vector *nodes, diag_vector *diags);
int is_arm_op(tktype t);
int read_arm_op(parser *p);

/* ---- struct / array (parser_decl.c / parser_expr.c) ---- */

int parse_structdef(parser *p, node_vector *nodes, diag_vector *diags);
int parse_structfield(parser *p, node_vector *nodes, diag_vector *diags);
int parse_structlit(parser *p, node_vector *nodes, diag_vector *diags,
                    int type_idx);
int parse_fieldaccess(parser *p, node_vector *nodes, diag_vector *diags,
                      int expr_idx);
int parse_arraylit(parser *p, node_vector *nodes, diag_vector *diags);

#endif /* PARSER_PARSER_INTERNAL_H */
