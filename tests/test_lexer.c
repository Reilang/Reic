/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * test_lexer.c — Characterization tests for the lexer (tokenize).
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
#include "lexer/lexer.h"
#include "test.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    lexer lexer_;
    token_vector tokens;
    diag_vector diags;
} LexFixture;

static void lex_init(LexFixture *fx, char *src_raw)
{
    memset(fx, 0, sizeof(*fx));
    fx->lexer_.src_.raw = src_raw;
    token_vec_new(&fx->tokens, 16);
    diag_vec_new(&fx->diags, 4);
}

static void lex_run(LexFixture *fx)
{
    tokenize(&fx->lexer_, &fx->tokens, &fx->diags);
}

static void lex_free(LexFixture *fx)
{
    token_vec_free(&fx->tokens);
    diag_vec_free(&fx->diags);
}

static void test_keywords(void)
{
    printf("--- test_keywords ---\n");
    char src_raw[] = "fn var if while loop return";
    LexFixture fx;
    lex_init(&fx, src_raw);
    lex_run(&fx);

    ASSERT_EQ_INT(fx.tokens.size, 7, "7 tokens (6 keywords + eof)");
    if (fx.tokens.size >= 6) {
        ASSERT(fx.tokens.data[0].type == TK_KEYWORD, "token 0 is KEYWORD(fn)");
        ASSERT(fx.tokens.data[1].type == TK_KEYWORD, "token 1 is KEYWORD(var)");
        ASSERT(fx.tokens.data[2].type == TK_KEYWORD, "token 2 is KEYWORD(if)");
        ASSERT(fx.tokens.data[3].type == TK_KEYWORD, "token 3 is KEYWORD(while)");
        ASSERT(fx.tokens.data[4].type == TK_KEYWORD, "token 4 is KEYWORD(loop)");
        ASSERT(fx.tokens.data[5].type == TK_KEYWORD, "token 5 is KEYWORD(return)");
    }

    lex_free(&fx);
}

static void test_integer_literal(void)
{
    printf("--- test_integer_literal ---\n");
    char src_raw[] = "42 0 999";
    LexFixture fx;
    lex_init(&fx, src_raw);
    lex_run(&fx);

    ASSERT_EQ_INT(fx.tokens.size, 4, "4 tokens (3 ints + eof)");
    if (fx.tokens.size >= 3) {
        ASSERT(fx.tokens.data[0].type == TK_ILITER, "token 0 is ILITER");
        ASSERT_EQ_INT((int)fx.tokens.data[0].value.integer, 42, "value 42");
        ASSERT(fx.tokens.data[1].type == TK_ILITER, "token 1 is ILITER");
        ASSERT_EQ_INT((int)fx.tokens.data[1].value.integer, 0, "value 0");
        ASSERT(fx.tokens.data[2].type == TK_ILITER, "token 2 is ILITER");
        ASSERT_EQ_INT((int)fx.tokens.data[2].value.integer, 999, "value 999");
    }

    lex_free(&fx);
}

static void test_operators(void)
{
    printf("--- test_operators ---\n");
    char src_raw[] = "+ - * / ( ) { } [ ] , : < > = !";
    LexFixture fx;
    lex_init(&fx, src_raw);
    lex_run(&fx);

    int expect = 17; /* 16 operators + eof */
    ASSERT_EQ_INT(fx.tokens.size, expect, "16 ops + 1 eof");
    if (fx.tokens.size >= 16) {
        ASSERT(fx.tokens.data[0].type == TK_ADD,       "token 0  +");
        ASSERT(fx.tokens.data[1].type == TK_MINUS,     "token 1  -");
        ASSERT(fx.tokens.data[2].type == TK_STAR,      "token 2  *");
        ASSERT(fx.tokens.data[3].type == TK_SLASH,     "token 3  /");
        ASSERT(fx.tokens.data[4].type == TK_OPAREN,    "token 4  (");
        ASSERT(fx.tokens.data[5].type == TK_CPAREN,    "token 5  )");
        ASSERT(fx.tokens.data[6].type == TK_OBRACE,    "token 6  {");
        ASSERT(fx.tokens.data[7].type == TK_CBRACE,    "token 7  }");
        ASSERT(fx.tokens.data[8].type == TK_OBRACKET,  "token 8  [");
        ASSERT(fx.tokens.data[9].type == TK_CBRACKET,  "token 9  ]");
        ASSERT(fx.tokens.data[10].type == TK_COMMA,    "token 10 ,");
        ASSERT(fx.tokens.data[11].type == TK_COLON,    "token 11 :");
        ASSERT(fx.tokens.data[12].type == TK_OABRACKET,"token 12 <");
        ASSERT(fx.tokens.data[13].type == TK_CABRACKET,"token 13 >");
        ASSERT(fx.tokens.data[14].type == TK_EQUAL,    "token 14 =");
        ASSERT(fx.tokens.data[15].type == TK_NOT,      "token 15 !");
    }

    lex_free(&fx);
}

static void test_identifiers(void)
{
    printf("--- test_identifiers ---\n");
    char src_raw[] = "foo bar_baz x1 _hidden";
    LexFixture fx;
    lex_init(&fx, src_raw);
    lex_run(&fx);

    ASSERT_EQ_INT(fx.tokens.size, 5, "4 idents + eof");
    if (fx.tokens.size >= 4) {
        ASSERT(fx.tokens.data[0].type == TK_IDENT, "token 0 is IDENT");
        ASSERT_STREQ(fx.tokens.data[0].value.string, "foo", "value foo");
        ASSERT(fx.tokens.data[1].type == TK_IDENT, "token 1 is IDENT");
        ASSERT_STREQ(fx.tokens.data[1].value.string, "bar_baz", "value bar_baz");
        ASSERT(fx.tokens.data[2].type == TK_IDENT, "token 2 is IDENT");
        ASSERT_STREQ(fx.tokens.data[2].value.string, "x1", "value x1");
        ASSERT(fx.tokens.data[3].type == TK_IDENT, "token 3 is IDENT");
        ASSERT_STREQ(fx.tokens.data[3].value.string, "_hidden", "value _hidden");
    }

    lex_free(&fx);
}

static void test_string_literal(void)
{
    printf("--- test_string_literal ---\n");
    char src_raw[] = "\"hello\" \"world\"";
    LexFixture fx;
    lex_init(&fx, src_raw);
    lex_run(&fx);

    ASSERT_EQ_INT(fx.tokens.size, 3, "2 strings + eof");
    if (fx.tokens.size >= 2) {
        ASSERT(fx.tokens.data[0].type == TK_SLITER, "token 0 is SLITER");
        ASSERT_STREQ(fx.tokens.data[0].value.string, "hello", "value hello");
        ASSERT(fx.tokens.data[1].type == TK_SLITER, "token 1 is SLITER");
        ASSERT_STREQ(fx.tokens.data[1].value.string, "world", "value world");
    }

    lex_free(&fx);
}

static void test_char_literal(void)
{
    printf("--- test_char_literal ---\n");
    char src_raw[] = "'x' '\\n'";
    LexFixture fx;
    lex_init(&fx, src_raw);
    lex_run(&fx);

    ASSERT_EQ_INT(fx.tokens.size, 3, "2 chars + eof");
    if (fx.tokens.size >= 2) {
        ASSERT(fx.tokens.data[0].type == TK_CLITER, "token 0 is CLITER");
        ASSERT_EQ_INT((int)fx.tokens.data[0].value.char_, 'x', "value 'x'");
        ASSERT(fx.tokens.data[1].type == TK_CLITER, "token 1 is CLITER");
        ASSERT_EQ_INT((int)fx.tokens.data[1].value.char_, '\n', "value '\\n'");
    }

    lex_free(&fx);
}

static void test_newlines(void)
{
    printf("--- test_newlines ---\n");
    char src_raw[] = "x\ny\nz";
    LexFixture fx;
    lex_init(&fx, src_raw);
    lex_run(&fx);

    /* x NEXTLINE y NEXTLINE z EOF */
    ASSERT_EQ_INT(fx.tokens.size, 6, "6 tokens (x nl y nl z eof)");
    if (fx.tokens.size >= 5) {
        ASSERT(fx.tokens.data[0].type == TK_IDENT,    "token 0 IDENT x");
        ASSERT(fx.tokens.data[1].type == TK_NEXTLINE, "token 1 NEXTLINE");
        ASSERT(fx.tokens.data[2].type == TK_IDENT,    "token 2 IDENT y");
        ASSERT(fx.tokens.data[3].type == TK_NEXTLINE, "token 3 NEXTLINE");
        ASSERT(fx.tokens.data[4].type == TK_IDENT,    "token 4 IDENT z");
    }

    lex_free(&fx);
}

static void test_comment_skip(void)
{
    printf("--- test_comment_skip ---\n");
    char src_raw[] = "x ; this is a comment\ny";
    LexFixture fx;
    lex_init(&fx, src_raw);
    lex_run(&fx);

    /* x NEXTLINE y EOF — the comment content is not a token */
    ASSERT_EQ_INT(fx.tokens.size, 4, "4 tokens (x nl y eof)");
    if (fx.tokens.size >= 3) {
        ASSERT(fx.tokens.data[0].type == TK_IDENT,    "token 0 IDENT x");
        ASSERT(fx.tokens.data[1].type == TK_NEXTLINE, "token 1 NEXTLINE");
        ASSERT(fx.tokens.data[2].type == TK_IDENT,    "token 2 IDENT y");
    }

    lex_free(&fx);
}

static void test_paren_suppresses_newline(void)
{
    printf("--- test_paren_suppresses_newline ---\n");
    char src_raw[] = "(\nx\n)";
    LexFixture fx;
    lex_init(&fx, src_raw);
    lex_run(&fx);

    /* ( x ) EOF — no NEXTLINE inside parens */
    ASSERT_EQ_INT(fx.tokens.size, 4, "4 tokens ( ( x ) eof ) — no NEXTLINE inside parens");
    if (fx.tokens.size >= 3) {
        ASSERT(fx.tokens.data[0].type == TK_OPAREN, "token 0 (");
        ASSERT(fx.tokens.data[1].type == TK_IDENT,  "token 1 IDENT x");
        ASSERT(fx.tokens.data[2].type == TK_CPAREN, "token 2 )");
    }

    lex_free(&fx);
}

int main(void)
{
    test_keywords();
    test_integer_literal();
    test_operators();
    test_identifiers();
    test_string_literal();
    test_char_literal();
    test_newlines();
    test_comment_skip();
    test_paren_suppresses_newline();

    TEST_SUMMARY();
    return TEST_RETURN();
}
