/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * test_codegen.c — Golden tests for the full compiler pipeline.
 *
 * Each test feeds source through lex -> parse -> sema -> lower -> codegen
 * and then checks that the emitted .ll file contains expected IR patterns.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "ast/ast.h"
#include "codegen/codegen.h"
#include "hir/hir.h"
#include "hir/hir_lower.h"
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

/* ---------- helpers ---------- */

#define GOLDEN_PATH "test_output.ll"

typedef struct {
    lexer lexer_;
    token_vector tokens;
    diag_vector diags;
    parser parser_;
    node_vector nodes;
    sema_vector annotations;
    hir_vector hir;
} GoldenFixture;

static void golden_init(GoldenFixture *fx, char *src_raw)
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

    /* sema */
    fx->annotations = sema_check(fx->nodes, &fx->diags);

    /* lower */
    fx->hir = hir_lower(fx->nodes, &fx->annotations);
}

static int golden_run(GoldenFixture *fx)
{
    bool has_errors = false;
    int i;
    for (i = 0; i < fx->diags.size; i++) {
        if (fx->diags.data[i].level_ == LEVEL_ERROR) {
            has_errors = true;
            break;
        }
    }
    if (has_errors) return -1;

    return codegen_emit(&fx->hir, GOLDEN_PATH);
}

static char *golden_read(const char *path)
{
    FILE *f = fopen(path, "r");
    long sz;
    char *buf;

    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    buf = (char *)malloc((size_t)(sz + 1));
    if (!buf) { fclose(f); return NULL; }

    fread(buf, 1, (size_t)sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

static void golden_free(GoldenFixture *fx)
{
    hir_vec_free(&fx->hir);
    sema_vec_free(&fx->annotations);
    state_vec_free(&fx->parser_.states);
    node_vec_free(&fx->nodes);
    token_vec_free(&fx->tokens);
    diag_vec_free(&fx->diags);
    remove(GOLDEN_PATH);
}

/* ---------- tests ---------- */

static void test_simple_func(void)
{
    printf("--- test_simple_func ---\n");
    char src_raw[] = "fn add(a: int32, b: int32) -> int32 {\n"
                 "  return a + b\n"
                 "}\n";
    GoldenFixture fx;
    golden_init(&fx, src_raw);

    ASSERT_EQ_INT(golden_run(&fx), 0, "codegen succeeds");

    char *ir = golden_read(GOLDEN_PATH);
    ASSERT(ir != NULL, "output file exists");
    if (!ir) { golden_free(&fx); return; }

    ASSERT(strstr(ir, "define i32 @add(i32 %a, i32 %b)") != NULL,
           "contains function definition");
    ASSERT(strstr(ir, "entry:") != NULL,
           "contains entry block");
    ASSERT(strstr(ir, "alloca") != NULL,
           "contains allocas");
    ASSERT(strstr(ir, "ret i32") != NULL,
           "contains return");

    free(ir);
    golden_free(&fx);
}

static void test_arithmetic(void)
{
    printf("--- test_arithmetic ---\n");
    char src_raw[] = "fn calc(x: int32) -> int32 {\n"
                 "  var y := x + 2\n"
                 "  y := y * 3\n"
                 "  return y\n"
                 "}\n";
    GoldenFixture fx;
    golden_init(&fx, src_raw);

    ASSERT_EQ_INT(golden_run(&fx), 0, "codegen succeeds");

    char *ir = golden_read(GOLDEN_PATH);
    ASSERT(ir != NULL, "output file exists");
    if (!ir) { golden_free(&fx); return; }

    ASSERT(strstr(ir, "add i32") != NULL, "contains add");
    ASSERT(strstr(ir, "mul i32") != NULL, "contains mul");
    ASSERT(strstr(ir, "ret i32") != NULL, "contains return");

    free(ir);
    golden_free(&fx);
}

static void test_const_inlining(void)
{
    printf("--- test_const_inlining ---\n");
    char src_raw[] = "fn test() -> int32 {\n"
                 "  X = 42\n"
                 "  return X\n"
                 "}\n";
    GoldenFixture fx;
    golden_init(&fx, src_raw);

    ASSERT_EQ_INT(golden_run(&fx), 0, "codegen succeeds");

    char *ir = golden_read(GOLDEN_PATH);
    ASSERT(ir != NULL, "output file exists");
    if (!ir) { golden_free(&fx); return; }

    ASSERT(strstr(ir, "ret i32 42") != NULL,
           "const inlined: ret i32 42");

    free(ir);
    golden_free(&fx);
}

static void test_if_with_switch(void)
{
    printf("--- test_if_with_switch ---\n");
    /* All arms use = (equality) => should generate a switch. */
    char src_raw[] = "fn test(x: int32) -> int32 {\n"
                 "  if (x) {\n"
                 "    = 1 =>\n"
                 "      return 10\n"
                 "    = 2 =>\n"
                 "      return 20\n"
                 "  }\n"
                 "  return 0\n"
                 "}\n";
    GoldenFixture fx;
    golden_init(&fx, src_raw);

    ASSERT_EQ_INT(golden_run(&fx), 0, "codegen succeeds");

    char *ir = golden_read(GOLDEN_PATH);
    ASSERT(ir != NULL, "output file exists");
    if (!ir) { golden_free(&fx); return; }

    /* Equality-only if should produce switch, not icmp chain. */
    ASSERT(strstr(ir, "switch i32") != NULL, "contains switch");

    free(ir);
    golden_free(&fx);
}

static void test_while_loop(void)
{
    printf("--- test_while_loop ---\n");
    char src_raw[] = "fn count(n: int32) -> int32 {\n"
                 "  var i := 0\n"
                 "  while (i < n) {\n"
                 "    i := i + 1\n"
                 "  }\n"
                 "  return i\n"
                 "}\n";
    GoldenFixture fx;
    golden_init(&fx, src_raw);

    ASSERT_EQ_INT(golden_run(&fx), 0, "codegen succeeds");

    char *ir = golden_read(GOLDEN_PATH);
    ASSERT(ir != NULL, "output file exists");
    if (!ir) { golden_free(&fx); return; }

    ASSERT(strstr(ir, "while_cond") != NULL, "contains while_cond label");
    ASSERT(strstr(ir, "while_body") != NULL, "contains while_body label");
    ASSERT(strstr(ir, "while_exit") != NULL, "contains while_exit label");

    free(ir);
    golden_free(&fx);
}

/* ---------- main ---------- */

int main(void)
{
    test_simple_func();
    test_arithmetic();
    test_const_inlining();
    test_if_with_switch();
    test_while_loop();

    TEST_SUMMARY();
    return TEST_RETURN();
}
