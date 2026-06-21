/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * token.c — Converts tokens to human-readable string representations.
 *
 * Implements token_print(), which allocates and returns a debug string
 * describing the token's type, value, and source position.
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
        snprintf(buffer, 256, "[%d:%d] identifier: %s", token_.line, token_.col,
                token_.value.string);
        break;
    case TK_KEYWORD:
        snprintf(buffer, 256, "[%d:%d] keyword: %s", token_.line, token_.col,
                token_.value.string);
        break;
    case TK_ILITER:
        snprintf(buffer, 256, "[%d:%d] integer: %lld", token_.line, token_.col,
                (long long)token_.value.integer);
        break;
    case TK_FLITER:
        snprintf(buffer, 256, "[%d:%d] float: %f", token_.line, token_.col,
                token_.value.float_);
        break;
    case TK_SLITER:
        snprintf(buffer, 256, "[%d:%d] string: \"%s\"", token_.line, token_.col,
                token_.value.string);
        break;
    case TK_CLITER:
        snprintf(buffer, 256, "[%d:%d] char: '%c'", token_.line, token_.col,
                token_.value.char_);
        break;
    case TK_ADD:
        snprintf(buffer, 256, "[%d:%d] +", token_.line, token_.col);
        break;
    case TK_MINUS:
        snprintf(buffer, 256, "[%d:%d] -", token_.line, token_.col);
        break;
    case TK_STAR:
        snprintf(buffer, 256, "[%d:%d] *", token_.line, token_.col);
        break;
    case TK_SLASH:
        snprintf(buffer, 256, "[%d:%d] /", token_.line, token_.col);
        break;
    case TK_EQUAL:
        snprintf(buffer, 256, "[%d:%d] =", token_.line, token_.col);
        break;
    case TK_OPAREN:
        snprintf(buffer, 256, "[%d:%d] (", token_.line, token_.col);
        break;
    case TK_CPAREN:
        snprintf(buffer, 256, "[%d:%d] )", token_.line, token_.col);
        break;
    case TK_COMMA:
        snprintf(buffer, 256, "[%d:%d] ,", token_.line, token_.col);
        break;
    case TK_OBRACKET:
        snprintf(buffer, 256, "[%d:%d] [", token_.line, token_.col);
        break;
    case TK_CBRACKET:
        snprintf(buffer, 256, "[%d:%d] ]", token_.line, token_.col);
        break;
    case TK_OBRACE:
        snprintf(buffer, 256, "[%d:%d] {", token_.line, token_.col);
        break;
    case TK_CBRACE:
        snprintf(buffer, 256, "[%d:%d] }", token_.line, token_.col);
        break;
    case TK_OABRACKET:
        snprintf(buffer, 256, "[%d:%d] <", token_.line, token_.col);
        break;
    case TK_CABRACKET:
        snprintf(buffer, 256, "[%d:%d] >", token_.line, token_.col);
        break;
    case TK_COLON:
        snprintf(buffer, 256, "[%d:%d] :", token_.line, token_.col);
        break;
    case TK_NOT:
        snprintf(buffer, 256, "[%d:%d] !", token_.line, token_.col);
        break;
    case TK_GREATEREQUAL:
        snprintf(buffer, 256, "[%d:%d] >=", token_.line, token_.col);
        break;
    case TK_LESSEQUAL:
        snprintf(buffer, 256, "[%d:%d] <=", token_.line, token_.col);
        break;
    case TK_NOTEQUAL:
        snprintf(buffer, 256, "[%d:%d] !=", token_.line, token_.col);
        break;
    case TK_NEXTLINE:
        snprintf(buffer, 256, "[%d:%d] <nextline>", token_.line, token_.col);
        break;
    case TK_EOF:
        snprintf(buffer, 256, "[%d:%d] <eof>", token_.line, token_.col);
        break;
    default:
        snprintf(buffer, 256, "[%d:%d] ???", token_.line, token_.col);
    }

    return buffer;
}

const char *binop_symbol(tktype op)
{
    switch (op) {
    case TK_ADD:           return "+";
    case TK_MINUS:         return "-";
    case TK_STAR:          return "*";
    case TK_SLASH:         return "/";
    case TK_EQUAL:         return "=";
    case TK_OABRACKET:     return "<";
    case TK_CABRACKET:     return ">";
    case TK_NOT:           return "!";
    case TK_GREATEREQUAL:  return ">=";
    case TK_LESSEQUAL:     return "<=";
    case TK_NOTEQUAL:      return "!=";
    default:               return "?";
    }
}
