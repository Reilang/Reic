/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * token.c — Converts tokens to human-readable string representations.
 *
 * Implements token_print(), which allocates and returns a debug string
 * describing the token's type, value, and source position.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include <stdio.h>
#include <stdlib.h>

#include "token/token.h"

char *token_print(token token_)
{
    char *buffer = NULL;

    buffer = (char*)malloc(256);
    if (!buffer)
        return NULL;

    switch (token_.type) {
    case TK_IDENT:
        snprintf(buffer, 256, "[%d:%d] identifier: %s", token_.line, token_.column,
                token_.value.string);
        break;
    case TK_IDENT_FUNC:
        snprintf(buffer, 256, "[%d:%d] function-name: %s", token_.line, token_.column,
                token_.value.string);
        break;
    case TK_IDENT_VAR:
        snprintf(buffer, 256, "[%d:%d] variable-name: %s", token_.line, token_.column,
                token_.value.string);
        break;
    case TK_IDENT_TYPE:
        snprintf(buffer, 256, "[%d:%d] type-name: %s", token_.line, token_.column,
                token_.value.string);
        break;
    case TK_KEYWORD:
        snprintf(buffer, 256, "[%d:%d] keyword: %s", token_.line, token_.column,
                token_.value.string);
        break;
    case TK_ILITER:
        snprintf(buffer, 256, "[%d:%d] integer: %lld", token_.line, token_.column,
                (long long)token_.value.integer);
        break;
    case TK_FLITER:
        snprintf(buffer, 256, "[%d:%d] float: %f", token_.line, token_.column,
                token_.value.float_);
        break;
    case TK_SLITER:
        snprintf(buffer, 256, "[%d:%d] string: \"%s\"", token_.line, token_.column,
                token_.value.string);
        break;
    case TK_CLITER:
        snprintf(buffer, 256, "[%d:%d] char: '%c'", token_.line, token_.column,
                token_.value.char_);
        break;
    case TK_ADD:
        snprintf(buffer, 256, "[%d:%d] +", token_.line, token_.column);
        break;
    case TK_MINUS:
        snprintf(buffer, 256, "[%d:%d] -", token_.line, token_.column);
        break;
    case TK_STAR:
        snprintf(buffer, 256, "[%d:%d] *", token_.line, token_.column);
        break;
    case TK_SLASH:
        snprintf(buffer, 256, "[%d:%d] /", token_.line, token_.column);
        break;
    case TK_EQUAL:
        snprintf(buffer, 256, "[%d:%d] =", token_.line, token_.column);
        break;
    case TK_OPAREN:
        snprintf(buffer, 256, "[%d:%d] (", token_.line, token_.column);
        break;
    case TK_CPAREN:
        snprintf(buffer, 256, "[%d:%d] )", token_.line, token_.column);
        break;
    case TK_OBRACKET:
        snprintf(buffer, 256, "[%d:%d] [", token_.line, token_.column);
        break;
    case TK_CBRACKET:
        snprintf(buffer, 256, "[%d:%d] ]", token_.line, token_.column);
        break;
    case TK_OBRACE:
        snprintf(buffer, 256, "[%d:%d] {", token_.line, token_.column);
        break;
    case TK_CBRACE:
        snprintf(buffer, 256, "[%d:%d] }", token_.line, token_.column);
        break;
    case TK_OABRACKET:
        snprintf(buffer, 256, "[%d:%d] <", token_.line, token_.column);
        break;
    case TK_CABRACKET:
        snprintf(buffer, 256, "[%d:%d] >", token_.line, token_.column);
        break;
    case TK_COLON:
        snprintf(buffer, 256, "[%d:%d] :", token_.line, token_.column);
        break;
    case TK_NEXTLINE:
        snprintf(buffer, 256, "[%d:%d] <nextline>", token_.line, token_.column);
        break;
    case TK_EOF:
        snprintf(buffer, 256, "[%d:%d] <eof>", token_.line, token_.column);
        break;
    default:
        snprintf(buffer, 256, "[%d:%d] ???", token_.line, token_.column);
    }

    return buffer;
}
