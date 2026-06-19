/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * main.c — Compiler entry point: lex -> parse -> print.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "ast/ast.h"
#include "codegen/codegen.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "sema/sema.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char src_raw[] = "fn main(a: int32, b: int32) -> int32 {\n"
                     "    var x: int32 := 1\n"
                     "    var y := x + 2\n"
                     "    x := x + y\n"
                     "    if (x) {\n"
                     "        = 10 =>\n"
                     "            var z := 42\n"
                     "            x := z\n"
                     "        < 20 =>\n"
                     "            return y\n"
                     "    }\n"
                     "    while (x < 100) {\n"
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

    /* lex */
    lexer_.src_.raw = src_raw;
    token_vec_new(&tokens, 16);
    diag_vec_new(&diags, 4);

    tokenize(&lexer_, &tokens, &diags);

    /* parse */
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

    /* semantic analysis */
    sema_check(nodes, &diags);

    printf("\nsema diagnostics (%d):\n", diags.size);
    for (i = 0; i < diags.size; i++) {
        char *s = diag_print(diags.data[i]);
        if (s) {
            printf("  %s\n", s);
            free(s);
        }
    }

    /* codegen */
    if (diags.size == 0) {
        if (codegen_emit(&nodes, "output.ll") == 0)
            printf("\n=> wrote output.ll\n");
        else
            printf("\n=> codegen failed\n");
    }

    /* cleanup */
    /* In fact, OS will do this:-) */
    state_vec_free(&parser_.states);
    node_vec_free(&nodes);
    token_vec_free(&tokens);
    diag_vec_free(&diags);

    return EXIT_SUCCESS;
}
