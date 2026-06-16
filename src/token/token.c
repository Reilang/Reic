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
    char *buffer;

    buffer = (char*)malloc(256);
    if (!buffer)
        return NULL;

    switch (token_.type) {
    case TK_IDENT:
        snprintf(buffer, 256, "identifier: %s", token_.value.string);
        break;
    case TK_ILITER:
        snprintf(buffer, 256, "integer: %lld", (long long)token_.value.integer);
        break;
    case TK_FLITER:
        snprintf(buffer, 256, "float: %f", token_.value.float_);
        break;
    case TK_SLITER:
        snprintf(buffer, 256, "string: \"%s\"", token_.value.string);
        break;
    case TK_CLITER:
        snprintf(buffer, 256, "char: '%c'", token_.value.char_);
        break;
    case TK_ADD:
        snprintf(buffer, 256, "+");
        break;
    case TK_MINUS:
        snprintf(buffer, 256, "-");
        break;
    case TK_STAR:
        snprintf(buffer, 256, "*");
        break;
    case TK_SLASH:
        snprintf(buffer, 256, "/");
        break;
    case TK_OPAREN:
        snprintf(buffer, 256, "(");
        break;
    case TK_CPAREN:
        snprintf(buffer, 256, ")");
        break;
    case TK_OBRACKET:
        snprintf(buffer, 256, "[");
        break;
    case TK_CBRACKET:
        snprintf(buffer, 256, "]");
        break;
    case TK_OBRACE:
        snprintf(buffer, 256, "{");
        break;
    case TK_CBRACE:
        snprintf(buffer, 256, "}");
        break;
    case TK_EOF:
        snprintf(buffer, 256, "<eof>");
        break;
    default:
        snprintf(buffer, 256, "???");
    }

    return buffer;
}
