/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * main.c — Compiler entry point: lex -> parse -> print.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "ast/ast.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char src_raw[] = "fn main() -> void {\n"
                     "    return\n"
                     "}\n";

    lexer lexer_;
    token_vector tokens;
    diag_vector diags;
    parser parser_;
    node_vector nodes;
    int i;

    /* lex */
    lexer_.src_.raw = src_raw;
    token_new(&tokens, 16);
    diag_new(&diags, 4);

    tokenize(&lexer_, &tokens, &diags);

    /* parse */
    parser_.tokens = tokens;
    parser_.cursor = 0;
    state_new(&parser_.states, 8);
    node_new(&nodes, 16);

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

    printf("\nnodes (%d):\n", nodes.size);
    for (i = 0; i < nodes.size; i++) {
        char *s = anode_print(nodes.data[i]);
        if (s) {
            printf("  %s\n", s);
            free(s);
        }
    }

    /* cleanup */
    /* In fact, OS will do this:-) */
    state_free(&parser_.states);
    node_free(&nodes);
    token_free(&tokens);
    diag_free(&diags);

    return EXIT_SUCCESS;
}
