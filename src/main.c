/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * main.c — Compiler entry point.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "lexer/lexer.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char src_raw[] = "()[]{}+-*/ // line comment\n"
                     "/* block comment */ ident\n"
                     "_foo bar123\n"
                     "42 3.14.1.1 0.5\n"
                     "\"hello\\nworld\" 'x' '\\t'\n";
    lexer lexer_;
    token_vector tokens;
    diag_vector diags;
    int i;

    lexer_.src_.raw = src_raw;
    lexer_.src_.line = 1;
    lexer_.src_.column = 0;
    token_new(&tokens, 16);
    diag_new(&diags, 4);

    tokenize(&lexer_, &tokens, &diags);

    printf("tokens (%d):\n", tokens.size);
    for (i = 0; i < tokens.size; i++) {
        char *s = token_print(tokens.data[i]);
        printf("  [%d:%d] %s\n", tokens.data[i].line, tokens.data[i].column, s);
        if (tokens.data[i].type == TK_IDENT || tokens.data[i].type == TK_SLITER)
            free(tokens.data[i].value.string);
        free(s);
    }

    printf("\ndiagnostics (%d):\n", diags.size);
    for (i = 0; i < diags.size; i++)
        printf("  [%d:%d] %s\n", diags.data[i].line, diags.data[i].column, diags.data[i].whaterr);

    token_free(&tokens);
    diag_free(&diags);
    return EXIT_SUCCESS;
}
