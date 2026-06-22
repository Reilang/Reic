/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * main.c — Compiler entry point: lex -> parse -> print.
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
#include "codegen/codegen.h"
#include "hir/hir.h"
#include "hir/hir_lower.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "sema/sema.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    type_sys_init();
    char src_raw[] = "fn main(a: int32, b: int32) -> int32 {\n"
                     "    MAX_VAL = 100\n"
                     "    var x: int32 := 1\n"
                     "    var y := x + MAX_VAL\n"
                     "    x := x + y\n"
                     "    if (x) {\n"
                     "        = 10 =>\n"
                     "            var z := 42\n"
                     "            x := z\n"
                     "        < 20 =>\n"
                     "            return y\n"
                     "    }\n"
                     "    while (x < MAX_VAL) {\n"
                     "        x := x + 1\n"
                     "    }\n"
                     "    loop {\n"
                     "        y := y - 1\n"
                     "    }\n"
                     "    return x\n"
                     "}\n";

    lexer lexer_;
    token_vector tokens;
    diag_vector diags;
    parser parser_;
    node_vector nodes;
    int i;

    lexer_.src_.raw = src_raw;
    token_vec_new(&tokens, 16);
    diag_vec_new(&diags, 4);

    tokenize(&lexer_, &tokens, &diags);

    parser_.tokens = tokens;
    parser_.cursor = 0;
    state_vec_new(&parser_.states, 8);
    node_vec_new(&nodes, 16);

    parse(&parser_, &nodes, &diags);

    printf("tokens (%d):\n", tokens.size);
    for (i = 0; i < tokens.size; i++) {
        char *s = token_print(tokens.data[i]);
        if (s) {
            printf("  %s\n", s);
            free(s);
        }
    }

    printf("\ndiagnostics (%d):\n", diags.size);
    for (i = 0; i < diags.size; i++) {
        char *s = diag_print(diags.data[i]);
        if (s) {
            printf("  %s\n", s);
            free(s);
        }
    }

    printf("\nAST:\n");
    {
        char *tree = ast_print_tree(nodes);
        if (tree) {
            printf("%s", tree);
            free(tree);
        }
    }

    sema_vector annot = sema_check(nodes, &diags);

    printf("\nsema diagnostics (%d):\n", diags.size);
    for (i = 0; i < diags.size; i++) {
        char *s = diag_print(diags.data[i]);
        if (s) {
            printf("  %s\n", s);
            free(s);
        }
    }

    hir_vector hir = hir_lower(nodes, &annot);
    printf("\nHIR:\n");
    {
        char *tree = hir_print_tree(hir);
        if (tree) {
            printf("%s", tree);
            free(tree);
        }
    }

    {
        bool has_errors = false;
        for (i = 0; i < diags.size; i++) {
            if (diags.data[i].level_ == LEVEL_ERROR) {
                has_errors = true;
                break;
            }
        }
        if (!has_errors) {
            if (codegen_emit(&hir, "output.ll") == 0)
                printf("\n=> wrote output.ll\n");
            else
                printf("\n=> codegen failed\n");
        }
    }

    state_vec_free(&parser_.states);
    node_vec_free(&nodes);
    token_vec_free(&tokens);
    diag_vec_free(&diags);
    sema_vec_free(&annot);
    hir_vec_free(&hir);

    return EXIT_SUCCESS;
}
