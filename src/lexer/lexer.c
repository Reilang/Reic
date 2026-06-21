/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * lexer.c — State-machine tokenizer driven by the lexer state field.
 *
 * Each iteration of the main loop dispatches on l->state.  Normal state skips
 * whitespace and routes into sub-states for identifiers, numeric literals,
 * strings, and character literals.  Error recovery is handled through the
 * dedicated error state.
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char KEYWORDS[64][64] = {"if", "for", "while", "loop", "return", "fn", "var"};

static char nextch(lexer *l)
{
    char c = *(l->src_.raw++);
    l->src_.col++;
    return c;
}

static void src_back(lexer *l)
{
    l->src_.raw--;
    l->src_.col--;
}

static char read_escape(lexer *l)
{
    char cur = nextch(l);
    switch (cur) {
    case 'n':  return '\n';
    case 't':  return '\t';
    case 'r':  return '\r';
    case '\\': return '\\';
    case '"':  return '"';
    case '\'': return '\'';
    case '0':  return '\0';
    default:   return cur;
    }
}

static void emit_tok(token_vector *tokens, tktype type, int line, int col)
{
    token tk = {0};
    tk.type = type;
    tk.line = line;
    tk.col = col;
    token_vec_push(tokens, tk);
}

static void emit_tok_str(token_vector *tokens, tktype type,
                         const char *str, int line, int col)
{
    token tk = {0};
    tk.type = type;
    tk.line = line;
    tk.col = col;
    tk.value.string = strdup(str);
    if (!tk.value.string)
        abort();
    token_vec_push(tokens, tk);
}

static void handle_newline(lexer *l, token_vector *tokens)
{
    if (l->paren_depth == 0)
        emit_tok(tokens, TK_NEXTLINE, l->src_.line, l->src_.col);
    l->src_.line++;
    l->src_.col = 0;
}

void tokenize(lexer *l, token_vector *tokens, diag_vector *diags)
{
    char cur = 0;
    int pos = 0;
    int sline = 0;
    int scol = 0;

    l->src_.line = 1;
    l->src_.col = 0;
    l->paren_depth = 0;
    l->state = LSTATE_NORMAL;

    while (l->state != LSTATE_END) {
        switch (l->state) {

        case LSTATE_NORMAL:
            sline = l->src_.line;
            scol = l->src_.col;
            cur = nextch(l);

            if (cur == ' ' || cur == '\t' || cur == '\r')
                break;

            if (cur == '\n') {
                handle_newline(l, tokens);
                break;
            }

            if (cur == '\0') {
                emit_tok(tokens, TK_EOF, l->src_.line, l->src_.col);
                l->state = LSTATE_END;
                break;
            }

            if (cur == '(' || cur == ')' || cur == '[' || cur == ']'
                || cur == '{' || cur == '}' || cur == ',' || cur == '+'
                || cur == '-' || cur == '*' || cur == '/' || cur == ':'
                || cur == '=' || cur == '<' || cur == '>' || cur == '!') {
                switch (cur) {
                case '+': emit_tok(tokens, TK_ADD,       sline, scol); break;
                case '-': emit_tok(tokens, TK_MINUS,     sline, scol); break;
                case '*': emit_tok(tokens, TK_STAR,      sline, scol); break;
                case '/': emit_tok(tokens, TK_SLASH,     sline, scol); break;
                case '=': emit_tok(tokens, TK_EQUAL,     sline, scol); break;
                case '(':
                    emit_tok(tokens, TK_OPAREN, sline, scol);
                    l->paren_depth++;
                    break;
                case ')':
                    emit_tok(tokens, TK_CPAREN, sline, scol);
                    if (l->paren_depth > 0)
                        l->paren_depth--;
                    break;
                case ',': emit_tok(tokens, TK_COMMA,     sline, scol); break;
                case '[': emit_tok(tokens, TK_OBRACKET,  sline, scol); break;
                case ']': emit_tok(tokens, TK_CBRACKET,  sline, scol); break;
                case '{': emit_tok(tokens, TK_OBRACE,    sline, scol); break;
                case '}': emit_tok(tokens, TK_CBRACE,    sline, scol); break;
                case ':': emit_tok(tokens, TK_COLON,     sline, scol); break;
                case '<': emit_tok(tokens, TK_OABRACKET, sline, scol); break;
                case '>': emit_tok(tokens, TK_CABRACKET, sline, scol); break;
                case '!': emit_tok(tokens, TK_NOT,       sline, scol); break;
                default:  l->state = LSTATE_ERROR; continue;
                }
                break;
            }

            if (cur == ';') {
                l->state = LSTATE_COMMENT;
                break;
            }

            if (isalpha(cur) || cur == '_') {
                pos = 0;
                l->rdbuf[pos++] = cur;
                sline = l->src_.line;
                scol = l->src_.col - 1;
                l->state = LSTATE_IDENT;
                break;
            }

            if (isdigit(cur)) {
                pos = 0;
                l->rdbuf[pos++] = cur;
                sline = l->src_.line;
                scol = l->src_.col - 1;
                l->state = LSTATE_ILITER;
                break;
            }

            if (cur == '"') {
                pos = 0;
                sline = l->src_.line;
                scol = l->src_.col - 1;
                l->state = LSTATE_SLITER;
                break;
            }

            if (cur == '\'') {
                sline = l->src_.line;
                scol = l->src_.col - 1;
                l->state = LSTATE_CLITER;
                break;
            }

            l->state = LSTATE_ERROR;
            break;

        case LSTATE_IDENT:
            cur = nextch(l);
            if (isalnum(cur) || cur == '_') {
                if (pos < 255)
                    l->rdbuf[pos++] = cur;
                break;
            }
            src_back(l);
            l->rdbuf[pos] = '\0';
            {
                int ki;
                tktype t = TK_IDENT;
                for (ki = 0; ki < 64; ki++) {
                    if (KEYWORDS[ki][0] == '\0')
                        continue;
                    if (strcmp(l->rdbuf, KEYWORDS[ki]) == 0) {
                        t = TK_KEYWORD;
                        break;
                    }
                }
                emit_tok_str(tokens, t, l->rdbuf, sline, scol);
            }
            l->state = LSTATE_NORMAL;
            break;

        case LSTATE_ILITER:
            cur = nextch(l);
            if (isdigit(cur)) {
                if (pos < 255)
                    l->rdbuf[pos++] = cur;
                break;
            }
            if (cur == '.') {
                if (pos < 255)
                    l->rdbuf[pos++] = cur;
                l->state = LSTATE_FLITER;
                break;
            }
            src_back(l);
            l->rdbuf[pos] = '\0';
            {
                token tk = {0};
                tk.type = TK_ILITER;
                tk.line = sline;
                tk.col = scol;
                tk.value.integer = (int64_t)strtoll(l->rdbuf, NULL, 10);
                token_vec_push(tokens, tk);
            }
            l->state = LSTATE_NORMAL;
            break;

        case LSTATE_FLITER:
            cur = nextch(l);
            if (isdigit(cur)) {
                if (pos < 255)
                    l->rdbuf[pos++] = cur;
                break;
            }
            if (cur == '.' && memchr(l->rdbuf, '.', (size_t)pos) != NULL) {
                diag_add(diags, LEVEL_ERROR, "duplicate dot in float literal",
                         sline, scol);
                while (*(l->src_.raw) && *(l->src_.raw) != ' '
                       && *(l->src_.raw) != '\t' && *(l->src_.raw) != '\n'
                       && *(l->src_.raw) != '\r') {
                    nextch(l);
                }
                goto emit_fliter;
            }
            if (pos > 0 && l->rdbuf[pos - 1] == '.') {
                {
                    char fbuf[128];
                    snprintf(fbuf, sizeof(fbuf),
                             "unexpected character '%c' (0x%02X) in float literal",
                             cur, (unsigned char)cur);
                    diag_add(diags, LEVEL_ERROR, fbuf, sline, scol);
                }
                if (cur == '\n') {
                    l->src_.line++;
                    l->src_.col = 0;
                }
                goto emit_fliter;
            }
            src_back(l);
        emit_fliter:
            l->rdbuf[pos] = '\0';
            {
                token tk = {0};
                tk.type = TK_FLITER;
                tk.line = sline;
                tk.col = scol;
                tk.value.float_ = strtod(l->rdbuf, NULL);
                token_vec_push(tokens, tk);
            }
            l->state = LSTATE_NORMAL;
            break;

        case LSTATE_SLITER:
            cur = nextch(l);
            if (cur == '\0' || cur == '\n') {
                diag_add(diags, LEVEL_ERROR, "unterminated string literal",
                         sline, scol);
                l->rdbuf[pos] = '\0';
                emit_tok_str(tokens, TK_SLITER, l->rdbuf, sline, scol);
                l->state = LSTATE_NORMAL;
                break;
            }
            if (cur == '"') {
                l->rdbuf[pos] = '\0';
                emit_tok_str(tokens, TK_SLITER, l->rdbuf, sline, scol);
                l->state = LSTATE_NORMAL;
                break;
            }
            if (cur == '\\') {
                cur = read_escape(l);
                if (pos < 255)
                    l->rdbuf[pos++] = cur;
                break;
            }
            if (pos < 255)
                l->rdbuf[pos++] = cur;
            break;

        case LSTATE_CLITER:
            cur = nextch(l);
            if (cur == '\0' || cur == '\n') {
                diag_add(diags, LEVEL_ERROR, "unterminated character literal",
                         sline, scol);
                l->state = LSTATE_NORMAL;
                break;
            }
            if (cur == '\'') {
                diag_add(diags, LEVEL_ERROR, "empty character literal",
                         sline, scol);
                emit_tok(tokens, TK_CLITER, sline, scol);
                tokens->data[tokens->size - 1].value.char_ = '\0';
                l->state = LSTATE_NORMAL;
                break;
            }
            if (cur == '\\') {
                cur = read_escape(l);
            }
            l->rdbuf[0] = cur;
            cur = nextch(l);
            if (cur != '\'') {
                diag_add(diags, LEVEL_ERROR, "unterminated character literal",
                         sline, scol);
                l->state = LSTATE_NORMAL;
                break;
            }
            emit_tok(tokens, TK_CLITER, sline, scol);
            tokens->data[tokens->size - 1].value.char_ = l->rdbuf[0];
            l->state = LSTATE_NORMAL;
            break;

        case LSTATE_COMMENT:
            cur = nextch(l);
            if (cur == '\0') {
                emit_tok(tokens, TK_EOF, l->src_.line, l->src_.col);
                l->state = LSTATE_END;
                break;
            }
            if (cur == '\n') {
                handle_newline(l, tokens);
                l->state = LSTATE_NORMAL;
            }
            break;

        case LSTATE_ERROR: {
            char efbuf[128];
            snprintf(efbuf, sizeof(efbuf),
                     "unexpected character '%c' (0x%02X)", cur, (unsigned char)cur);
            diag_add(diags, LEVEL_ERROR, efbuf, sline, scol);
        }
            l->state = LSTATE_NORMAL;
            break;

        case LSTATE_END:
            break;
        }
    }
}
