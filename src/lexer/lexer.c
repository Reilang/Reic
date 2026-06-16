/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * lexer.c — Goto-based state machine that tokenizes raw source code.
 *
 * Implements a hand-written lexer driven by goto labels for each state:
 * identifier scanning, numeric literal parsing (integer / float), string
 * and character literal handling with escape sequences, comment skipping
 * (line and block), and simple single-character token dispatch.  Errors are
 * reported as diagnostics attached to the diag_vector.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "lexer/lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void tokenize(lexer *lexer_, token_vector *tokens, diag_vector *diags)
{
    char cur;
    int pos;
    int sline;
    int scol;
    token tk;
    diag dg;

    lexer_->src_.line = 1;
    lexer_->src_.column = 0;

start:
    sline = lexer_->src_.line;
    scol = lexer_->src_.column;
    cur = *(lexer_->src_.raw++);
    lexer_->src_.column++;

    if (cur == ' ' || cur == '\t' || cur == '\r')
        goto start;

    if (cur == '\n') {
        lexer_->src_.line++;
        lexer_->src_.column = 0;
        goto start;
    }

    if (cur == '\0')
        goto emit_eof;

    if (cur == '(' || cur == ')' || cur == '[' || cur == ']'
        || cur == '{' || cur == '}' || cur == '+' || cur == '-' || cur == '*')
        goto emit_simple;

    if (cur == '/')
        goto slash_or_comment;

    pos = 0;
    lexer_->readnow[pos++] = cur;

    if (cur == '"')
        goto string_start;

    if (cur == '\'')
        goto char_start;

    if (isalpha(cur) || cur == '_')
        goto ident_loop;

    if (isdigit(cur))
        goto number_loop;

    goto emit_error;

ident_loop:
    sline = lexer_->src_.line;
    scol = lexer_->src_.column - 1;
    pos = 0;
    lexer_->readnow[pos++] = cur;
    goto ident_next;

ident_next:
    cur = *(lexer_->src_.raw++);
    lexer_->src_.column++;
    if (isalnum(cur) || cur == '_') {
        if (pos < 255)
            lexer_->readnow[pos++] = cur;
        goto ident_next;
    }
    lexer_->src_.raw--;
    lexer_->src_.column--;
    lexer_->readnow[pos] = '\0';
    goto emit_ident;

number_loop:
    sline = lexer_->src_.line;
    scol = lexer_->src_.column - 1;
    pos = 0;
    lexer_->readnow[pos++] = cur;
    goto number_next;

number_next:
    cur = *(lexer_->src_.raw++);
    lexer_->src_.column++;
    if (isdigit(cur)) {
        if (pos < 255)
            lexer_->readnow[pos++] = cur;
        goto number_next;
    }
    if (cur == '.') {
        if (pos < 255)
            lexer_->readnow[pos++] = cur;
        goto float_need_digit;
    }
    lexer_->src_.raw--;
    lexer_->src_.column--;
    lexer_->readnow[pos] = '\0';
    goto emit_iliter;

float_need_digit:
    cur = *(lexer_->src_.raw++);
    lexer_->src_.column++;
    if (cur == '.') 
        goto emit_duplicate_dot;
    if (!isdigit(cur))
        goto emit_error;
    if (pos < 255)
        lexer_->readnow[pos++] = cur;
    goto float_loop;

float_loop:
    cur = *(lexer_->src_.raw++);
    lexer_->src_.column++;
    if (isdigit(cur)) {
        if (pos < 255)
            lexer_->readnow[pos++] = cur;
        goto float_loop;
    }
    lexer_->src_.raw--;
    lexer_->src_.column--;
    lexer_->readnow[pos] = '\0';
    goto emit_fliter;

string_start:
    pos = 0;
    sline = lexer_->src_.line;
    scol = lexer_->src_.column - 1;
    goto string_next;

string_next:
    cur = *(lexer_->src_.raw++);
    lexer_->src_.column++;
    if (cur == '\0' || cur == '\n')
        goto emit_unterminated_str;
    if (cur == '"')
        goto emit_sliteral;
    if (cur == '\\')
        goto string_escape;
    if (pos < 255)
        lexer_->readnow[pos++] = cur;
    goto string_next;

string_escape:
    cur = *(lexer_->src_.raw++);
    lexer_->src_.column++;
    switch (cur) {
    case 'n':  cur = '\n'; break;
    case 't':  cur = '\t'; break;
    case 'r':  cur = '\r'; break;
    case '\\': cur = '\\'; break;
    case '"':  cur = '"';  break;
    case '\'': cur = '\''; break;
    case '0':  cur = '\0'; break;
    default:                 break;
    }
    if (pos < 255)
        lexer_->readnow[pos++] = cur;
    goto string_next;

char_start:
    sline = lexer_->src_.line;
    scol = lexer_->src_.column - 1;
    goto char_next;

char_next:
    cur = *(lexer_->src_.raw++);
    lexer_->src_.column++;
    if (cur == '\0' || cur == '\n')
        goto emit_unterminated_char;
    if (cur == '\'')
        goto emit_empty_char;
    if (cur == '\\')
        goto char_escape;
    lexer_->readnow[0] = cur;
    pos = 1;
    goto char_close;

char_escape:
    cur = *(lexer_->src_.raw++);
    lexer_->src_.column++;
    switch (cur) {
    case 'n':  cur = '\n'; break;
    case 't':  cur = '\t'; break;
    case 'r':  cur = '\r'; break;
    case '\\': cur = '\\'; break;
    case '\'': cur = '\''; break;
    case '"':  cur = '"';  break;
    case '0':  cur = '\0'; break;
    default:                 break;
    }
    lexer_->readnow[0] = cur;
    pos = 1;
    goto char_close;

char_close:
    cur = *(lexer_->src_.raw++);
    lexer_->src_.column++;
    if (cur != '\'')
        goto emit_unterminated_char;
    goto emit_cliteral;

slash_or_comment:
    cur = *(lexer_->src_.raw++);
    lexer_->src_.column++;
    if (cur == '/')
        goto skip_line_comment;
    if (cur == '*')
        goto skip_block_comment;
    lexer_->src_.raw--;
    lexer_->src_.column--;
    cur = '/';
    goto emit_simple;

skip_line_comment:
    cur = *(lexer_->src_.raw++);
    lexer_->src_.column++;
    if (cur == '\0')
        goto emit_eof;
    if (cur == '\n') {
        lexer_->src_.line++;
        lexer_->src_.column = 0;
        goto start;
    }
    goto skip_line_comment;

skip_block_comment:
    cur = *(lexer_->src_.raw++);
    lexer_->src_.column++;
    if (cur == '\0')
        goto emit_unterminated_comment;
    if (cur == '\n') {
        lexer_->src_.line++;
        lexer_->src_.column = 0;
        goto skip_block_comment;
    }
    if (cur == '*') {
        cur = *(lexer_->src_.raw++);
        lexer_->src_.column++;
        if (cur == '/')
            goto start;
        if (cur == '\0')
            goto emit_unterminated_comment;
        lexer_->src_.raw--;
        lexer_->src_.column--;
    }
    goto skip_block_comment;

skip_redundant_fliter:
    cur = *(lexer_->src_.raw++);
    lexer_->src_.column++;
    if (isdigit(cur))
        goto skip_redundant_fliter;
    lexer_->src_.raw--;
    lexer_->src_.column--;
    goto start;

emit_unterminated_comment:
    snprintf(dg.whaterr, sizeof(dg.whaterr), "unterminated block comment");
    dg.line = sline;
    dg.column = scol;
    dg.level_ = LEVEL_ERROR;
    diag_push(diags, dg);
    goto start;

emit_simple:
    tk.line = sline;
    tk.column = scol;
    switch (cur) {
    case '+': tk.type = TK_ADD;      break;
    case '-': tk.type = TK_MINUS;    break;
    case '*': tk.type = TK_STAR;     break;
    case '/': tk.type = TK_SLASH;    break;
    case '(': tk.type = TK_OPAREN;   break;
    case ')': tk.type = TK_CPAREN;   break;
    case '[': tk.type = TK_OBRACKET; break;
    case ']': tk.type = TK_CBRACKET; break;
    case '{': tk.type = TK_OBRACE;   break;
    case '}': tk.type = TK_CBRACE;   break;
    default:  goto emit_error;
    }
    token_push(tokens, tk);
    goto start;

emit_ident:
    tk.type = TK_IDENT;
    tk.line = sline;
    tk.column = scol;
    tk.value.string = strdup(lexer_->readnow);
    if (!tk.value.string)
        abort();
    token_push(tokens, tk);
    goto start;

emit_iliter:
    tk.type = TK_ILITER;
    tk.line = sline;
    tk.column = scol;
    tk.value.integer = (int64_t)strtoll(lexer_->readnow, NULL, 10);
    token_push(tokens, tk);
    goto start;

emit_fliter:
    tk.type = TK_FLITER;
    tk.line = sline;
    tk.column = scol;
    tk.value.float_ = strtod(lexer_->readnow, NULL);
    token_push(tokens, tk);
    goto start;

emit_sliteral:
    lexer_->readnow[pos] = '\0';
    tk.type = TK_SLITER;
    tk.line = sline;
    tk.column = scol;
    tk.value.string = strdup(lexer_->readnow);
    if (!tk.value.string)
        abort();
    token_push(tokens, tk);
    goto start;

emit_cliteral:
    tk.type = TK_CLITER;
    tk.line = sline;
    tk.column = scol;
    tk.value.char_ = lexer_->readnow[0];
    token_push(tokens, tk);
    goto start;

emit_duplicate_dot:
    snprintf(dg.whaterr, sizeof(dg.whaterr), "duplicate dot in float literal");
    dg.line = sline;
    dg.column = scol;
    dg.level_ = LEVEL_ERROR;
    diag_push(diags, dg);
    tk.type = TK_FLITER;
    tk.line = sline;
    tk.column = scol;
    tk.value.float_ = strtod(lexer_->readnow, NULL);
    goto skip_redundant_fliter;

emit_empty_char:
    snprintf(dg.whaterr, sizeof(dg.whaterr), "empty character literal");
    dg.line = sline;
    dg.column = scol;
    dg.level_ = LEVEL_ERROR;
    diag_push(diags, dg);
    tk.type = TK_CLITER;
    tk.line = sline;
    tk.column = scol;
    tk.value.char_ = '\0';
    token_push(tokens, tk);
    goto start;

emit_unterminated_str:
    snprintf(dg.whaterr, sizeof(dg.whaterr), "unterminated string literal");
    dg.line = sline;
    dg.column = scol;
    dg.level_ = LEVEL_ERROR;
    diag_push(diags, dg);
    lexer_->readnow[pos] = '\0';
    tk.type = TK_SLITER;
    tk.line = sline;
    tk.column = scol;
    tk.value.string = strdup(lexer_->readnow);
    if (!tk.value.string)
        abort();
    token_push(tokens, tk);
    goto start;

emit_unterminated_char:
    snprintf(dg.whaterr, sizeof(dg.whaterr), "unterminated character literal");
    dg.line = sline;
    dg.column = scol;
    dg.level_ = LEVEL_ERROR;
    diag_push(diags, dg);
    goto start;

emit_error:
    snprintf(dg.whaterr, sizeof(dg.whaterr), "unexpected character '%c' (0x%02X)", cur, (unsigned char)cur);
    dg.line = sline;
    dg.column = scol;
    dg.level_ = LEVEL_ERROR;
    diag_push(diags, dg);
    goto start;

emit_eof:
    tk.type = TK_EOF;
    tk.line = lexer_->src_.line;
    tk.column = lexer_->src_.column;
    token_push(tokens, tk);
    goto done;

done:
    return;
}
