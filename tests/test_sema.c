/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * test_sema.c — Characterization tests for semantic analysis (sema_check).
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
#include "diag/diag.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "sema/sema.h"
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
    sema_vector annot;
} SemaFixture;

static void sema_init(SemaFixture *fx, char *src_raw)
{
    memset(fx, 0, sizeof(*fx));

    /* lex */
    fx->lexer_.src_.raw = src_raw;
    token_vec_new(&fx->tokens, 16);
    diag_vec_new(&fx->diags, 4);
    tokenize(&fx->lexer_, &fx->tokens, &fx->diags);

    /* parse */
    fx->parser_.tokens = fx->tokens;
    fx->parser_.cursor = 0;
    state_vec_new(&fx->parser_.states, 8);
    node_vec_new(&fx->nodes, 16);
    parse(&fx->parser_, &fx->nodes, &fx->diags);
}

static void sema_run(SemaFixture *fx)
{
    fx->annot = sema_check(fx->nodes, &fx->diags);
}

static void sema_free(SemaFixture *fx)
{
    sema_vec_free(&fx->annot);
    state_vec_free(&fx->parser_.states);
    node_vec_free(&fx->nodes);
    token_vec_free(&fx->tokens);
    diag_vec_free(&fx->diags);
}

/* Count diagnostics at or above the given level. */
static int count_diags_at(const diag_vector *diags, level lv)
{
    int i, n = 0;
    for (i = 0; i < diags->size; i++)
        if (diags->data[i].level_ >= lv) n++;
    return n;
}

static void test_var_resolve_ok(void)
{
    printf("--- test_var_resolve_ok ---\n");
    char src_raw[] = "fn test() -> int32 {\n  var x: int32 := 1\n  return x\n}\n";
    SemaFixture fx;
    sema_init(&fx, src_raw);
    sema_run(&fx);

    ASSERT_EQ_INT(count_diags_at(&fx.diags, LEVEL_ERROR), 0,
                  "no errors on valid var use");

    sema_free(&fx);
}

static void test_undeclared_var(void)
{
    printf("--- test_undeclared_var ---\n");
    char src_raw[] = "fn test() -> int32 {\n  return x\n}\n";
    SemaFixture fx;
    sema_init(&fx, src_raw);
    sema_run(&fx);

    ASSERT(count_diags_at(&fx.diags, LEVEL_ERROR) > 0,
           "error on undeclared variable");

    sema_free(&fx);
}

static void test_duplicate_var(void)
{
    printf("--- test_duplicate_var ---\n");
    char src_raw[] = "fn test() -> int32 {\n  var x: int32 := 1\n  var x: int32 := 2\n"
                 "  return x\n}\n";
    SemaFixture fx;
    sema_init(&fx, src_raw);
    sema_run(&fx);

    ASSERT(count_diags_at(&fx.diags, LEVEL_ERROR) > 0,
           "error on duplicate variable declaration");

    sema_free(&fx);
}

static void test_type_mismatch_assign(void)
{
    printf("--- test_type_mismatch_assign ---\n");
    char src_raw[] = "fn test() -> void {\n  var x: int32 := 1\n  x := 2\n}\n";
    SemaFixture fx;
    sema_init(&fx, src_raw);
    sema_run(&fx);

    ASSERT_EQ_INT(count_diags_at(&fx.diags, LEVEL_ERROR), 0,
                  "no errors on same-type assign");

    sema_free(&fx);
}

static void test_constdecl_resolve(void)
{
    printf("--- test_constdecl_resolve ---\n");
    char src_raw[] = "fn test() -> int32 {\n  X = 100\n  return X\n}\n";
    SemaFixture fx;
    sema_init(&fx, src_raw);
    sema_run(&fx);

    ASSERT_EQ_INT(count_diags_at(&fx.diags, LEVEL_ERROR), 0,
                  "no errors on const declaration + use");

    sema_free(&fx);
}

static void test_unused_var_warning(void)
{
    printf("--- test_unused_var_warning ---\n");
    char src_raw[] = "fn test() -> void {\n  var x: int32 := 1\n}\n";
    SemaFixture fx;
    sema_init(&fx, src_raw);
    sema_run(&fx);

    ASSERT(count_diags_at(&fx.diags, LEVEL_WARN) > 0,
           "warning on unused variable");

    sema_free(&fx);
}

static void test_if_arm_scope(void)
{
    printf("--- test_if_arm_scope ---\n");
    /* variable declared inside an if-arm should not leak to another arm. */
    char src_raw[] = "fn test() -> void {\n"
                 "  var x: int32 := 0\n"
                 "  if (x) {\n"
                 "    = 0 =>\n"
                 "      var a := 1\n"
                 "      x := a\n"
                 "    < 0 =>\n"
                 "      x := 2\n"
                 "  }\n"
                 "}\n";
    SemaFixture fx;
    sema_init(&fx, src_raw);
    sema_run(&fx);

    ASSERT_EQ_INT(count_diags_at(&fx.diags, LEVEL_ERROR), 0,
                  "no errors: arm-scoped var not visible in other arm");

    sema_free(&fx);
}

int main(void)
{
    test_var_resolve_ok();
    test_undeclared_var();
    test_duplicate_var();
    test_type_mismatch_assign();
    test_constdecl_resolve();
    test_unused_var_warning();
    test_if_arm_scope();

    TEST_SUMMARY();
    return TEST_RETURN();
}
