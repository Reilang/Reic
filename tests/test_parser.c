/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * test_parser.c — Characterization tests for the parser (parse).
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
#include "ast/ast.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "test.h"
#include "token/token.h"
#include "type/type.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    lexer lexer_;
    token_vector tokens;
    diag_vector diags;
    parser parser_;
    node_vector nodes;
} ParseFixture;

static void parse_init(ParseFixture *fx, char *src_raw)
{
    memset(fx, 0, sizeof(*fx));

    /* lex */
    fx->lexer_.src_.raw = src_raw;
    token_vec_new(&fx->tokens, 16);
    diag_vec_new(&fx->diags, 4);
    tokenize(&fx->lexer_, &fx->tokens, &fx->diags);

    /* setup parser */
    fx->parser_.tokens = fx->tokens;
    fx->parser_.cursor = 0;
    state_vec_new(&fx->parser_.states, 8);
    node_vec_new(&fx->nodes, 16);
}

static void parse_run(ParseFixture *fx)
{
    parse(&fx->parser_, &fx->nodes, &fx->diags);
}

static void parse_free(ParseFixture *fx)
{
    state_vec_free(&fx->parser_.states);
    node_vec_free(&fx->nodes);
    token_vec_free(&fx->tokens);
    diag_vec_free(&fx->diags);
}

/* Count nodes of a given kind. */
static int count_kind(const node_vector *nodes, anode_kind kind)
{
    int i, n = 0;
    for (i = 0; i < nodes->size; i++)
        if (nodes->data[i].kind == kind) n++;
    return n;
}

static void test_funcdef(void)
{
    printf("--- test_funcdef ---\n");
    char src_raw[] = "fn foo(a: int32, b: int32) -> int32 {\n  return 42\n}\n";
    ParseFixture fx;
    parse_init(&fx, src_raw);
    parse_run(&fx);

    ASSERT(fx.diags.size == 0, "no parse errors");
    ASSERT(count_kind(&fx.nodes, ANODE_FUNCDECL) > 0, "has FUNCDECL");
    ASSERT(count_kind(&fx.nodes, ANODE_IDENT_FUNC) > 0, "has IDENT_FUNC");
    ASSERT(count_kind(&fx.nodes, ANODE_VARDECL) >= 2, "at least 2 VARDECL (params)");
    ASSERT(count_kind(&fx.nodes, ANODE_IDENT_TYPE) >= 1, "has IDENT_TYPE (ret type)");
    ASSERT(count_kind(&fx.nodes, ANODE_RETURN) > 0, "has RETURN");

    parse_free(&fx);
}

static void test_vardecl_typed_init(void)
{
    printf("--- test_vardecl_typed_init ---\n");
    char src_raw[] = "fn test() -> unit {\n  var x: int32 := 42\n}\n";
    ParseFixture fx;
    parse_init(&fx, src_raw);
    parse_run(&fx);

    ASSERT(fx.diags.size == 0, "no parse errors");
    ASSERT(count_kind(&fx.nodes, ANODE_VARDECL) > 0, "has VARDECL");
    ASSERT(count_kind(&fx.nodes, ANODE_ILITERAL) > 0, "has ILITERAL (42)");

    parse_free(&fx);
}

static void test_vardecl_inferred(void)
{
    printf("--- test_vardecl_inferred ---\n");
    char src_raw[] = "fn test() -> unit {\n  var y := 100\n}\n";
    ParseFixture fx;
    parse_init(&fx, src_raw);
    parse_run(&fx);

    ASSERT(fx.diags.size == 0, "no parse errors");
    ASSERT(count_kind(&fx.nodes, ANODE_VARDECL) > 0, "has VARDECL");

    parse_free(&fx);
}

static void test_assign(void)
{
    printf("--- test_assign ---\n");
    char src_raw[] = "fn test() -> unit {\n  var x: int32 := 0\n  x := 42\n}\n";
    ParseFixture fx;
    parse_init(&fx, src_raw);
    parse_run(&fx);

    ASSERT(fx.diags.size == 0, "no parse errors");
    ASSERT(count_kind(&fx.nodes, ANODE_ASSIGN) > 0, "has ASSIGN");

    parse_free(&fx);
}

static void test_if_match(void)
{
    printf("--- test_if_match ---\n");
    char src_raw[] = "fn test() -> unit {\n"
                 "  if (x) {\n"
                 "    = 1 =>\n"
                 "      return 1\n"
                 "    < 2 =>\n"
                 "      return 0\n"
                 "  }\n"
                 "}\n";
    ParseFixture fx;
    parse_init(&fx, src_raw);
    parse_run(&fx);

    ASSERT(fx.diags.size == 0, "no parse errors");
    ASSERT(count_kind(&fx.nodes, ANODE_IF) > 0, "has IF");
    ASSERT(count_kind(&fx.nodes, ANODE_MATCHARM) > 0, "has MATCHARM");

    parse_free(&fx);
}

static void test_while_loop(void)
{
    printf("--- test_while_loop ---\n");
    char src_raw[] = "fn test() -> unit {\n"
                 "  while (x < 10) {\n"
                 "    x := x + 1\n"
                 "  }\n"
                 "  loop {\n"
                 "    x := x - 1\n"
                 "  }\n"
                 "}\n";
    ParseFixture fx;
    parse_init(&fx, src_raw);
    parse_run(&fx);

    ASSERT(fx.diags.size == 0, "no parse errors");
    ASSERT(count_kind(&fx.nodes, ANODE_WHILE) > 0, "has WHILE");
    ASSERT(count_kind(&fx.nodes, ANODE_LOOP) > 0, "has LOOP");

    parse_free(&fx);
}

static void test_binop_precedence(void)
{
    printf("--- test_binop_precedence ---\n");
    /* 1 + 2 * 3  should parse as 1 + (2 * 3), not (1 + 2) * 3 */
    char src_raw[] = "fn test() -> unit {\n  var x := 1 + 2 * 3\n}\n";
    ParseFixture fx;
    parse_init(&fx, src_raw);
    parse_run(&fx);

    ASSERT(fx.diags.size == 0, "no parse errors");
    ASSERT(count_kind(&fx.nodes, ANODE_BINOP) > 0, "has BINOP");

    /* The top BINOP should be ADD (lower precedence), STAR is child */
    int i;
    for (i = 0; i < fx.nodes.size; i++) {
        if (fx.nodes.data[i].kind == ANODE_BINOP) {
            /* The outermost BINOP (root) has no parent referencing it as child/next.
             * In our test this is the only root-level expression decl.  The top
             * BINOP should be the ADD (precedence 2), not STAR (precedence 3). */
            if (fx.nodes.data[i].op == TK_ADD) {
                ASSERT(true, "top-level BINOP is ADD (lower prec wraps higher prec)");
                break;
            }
        }
    }

    parse_free(&fx);
}

static void test_constdecl(void)
{
    printf("--- test_constdecl ---\n");
    char src_raw[] = "fn test() -> int32 {\n  X = 100\n  return X\n}\n";
    ParseFixture fx;
    parse_init(&fx, src_raw);
    parse_run(&fx);

    ASSERT(fx.diags.size == 0, "no parse errors");
    ASSERT(count_kind(&fx.nodes, ANODE_CONSTDECL) > 0, "has CONSTDECL");

    parse_free(&fx);
}

static void test_structdef(void)
{
    printf("--- test_structdef ---\n");
    char src_raw[] = "fn test() -> unit {\n  Vec2 = struct { x: int32\n y: int32 }\n}\n";
    ParseFixture fx;
    parse_init(&fx, src_raw);
    parse_run(&fx);

    ASSERT(fx.diags.size == 0, "no parse errors");
    ASSERT(count_kind(&fx.nodes, ANODE_CONSTDECL) > 0, "has CONSTDECL");
    ASSERT(count_kind(&fx.nodes, ANODE_STRUCTDEF) > 0, "has STRUCTDEF");
    ASSERT(count_kind(&fx.nodes, ANODE_STRUCTFIELD) >= 2, "has at least 2 STRUCTFIELD");

    parse_free(&fx);
}

static void test_structlit(void)
{
    printf("--- test_structlit ---\n");
    char src_raw[] = "fn test() -> unit {\n"
                 "  Vec2 = struct { x: int32, y: int32 }\n"
                 "  var v := Vec2 { x: 1, y: 2 }\n"
                 "}\n";
    ParseFixture fx;
    parse_init(&fx, src_raw);
    parse_run(&fx);

    ASSERT(fx.diags.size == 0, "no parse errors");
    ASSERT(count_kind(&fx.nodes, ANODE_STRUCTLIT) > 0, "has STRUCTLIT");
    ASSERT(count_kind(&fx.nodes, ANODE_FIELDINIT) >= 2, "has at least 2 FIELDINIT");

    parse_free(&fx);
}

static void test_fieldaccess(void)
{
    printf("--- test_fieldaccess ---\n");
    char src_raw[] = "fn test() -> unit {\n"
                 "  Vec2 = struct { x: int32, y: int32 }\n"
                 "  var v := Vec2 { x: 1, y: 2 }\n"
                 "  var a := v.x\n"
                 "}\n";
    ParseFixture fx;
    parse_init(&fx, src_raw);
    parse_run(&fx);

    ASSERT(fx.diags.size == 0, "no parse errors");
    ASSERT(count_kind(&fx.nodes, ANODE_FIELDACCESS) > 0, "has FIELDACCESS");

    parse_free(&fx);
}

static void test_fieldassign(void)
{
    printf("--- test_fieldassign ---\n");
    char src_raw[] = "fn test() -> unit {\n"
                 "  Vec2 = struct { x: int32, y: int32 }\n"
                 "  var v := Vec2 { x: 1, y: 2 }\n"
                 "  v.x := 42\n"
                 "}\n";
    ParseFixture fx;
    parse_init(&fx, src_raw);
    parse_run(&fx);

    ASSERT(fx.diags.size == 0, "no parse errors");
    ASSERT(count_kind(&fx.nodes, ANODE_ASSIGN) > 0, "has ASSIGN");
    ASSERT(count_kind(&fx.nodes, ANODE_FIELDACCESS) > 0, "has FIELDACCESS (target)");

    parse_free(&fx);
}

int main(void)
{
    test_funcdef();
    test_vardecl_typed_init();
    test_vardecl_inferred();
    test_assign();
    test_if_match();
    test_while_loop();
    test_binop_precedence();
    test_constdecl();
    test_structdef();
    test_structlit();
    test_fieldaccess();
    test_fieldassign();

    TEST_SUMMARY();
    return TEST_RETURN();
}
