/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * lexer.c — State-machine tokenizer driven by the lexer state field.
 *
 * Each iteration of the main loop dispatches on lexer_->state.  Normal
 * state skips whitespace and routes into sub-states for identifiers,
 * numeric literals, strings, and character literals.  Error recovery is
 * handled through the dedicated error state.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "lexer/lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void tokenize(lexer *lexer_, token_vector *tokens, diag_vector *diags)
{
    char cur = 0;
    int pos = 0;
    int sline = 0;
    int scol = 0;
    token tk = {0};
    diag dg = {0};

    lexer_->src_.line = 1;
    lexer_->src_.column = 0;
    lexer_->state = LSTATE_NORMAL;

    while (lexer_->state != LSTATE_END) {
        switch (lexer_->state) {

        case LSTATE_NORMAL:
            sline = lexer_->src_.line;
            scol = lexer_->src_.column;
            cur = *(lexer_->src_.raw++);
            lexer_->src_.column++;

            if (cur == ' ' || cur == '\t' || cur == '\r')
                break;

            if (cur == '\n') {
                tk.type = TK_NEXTLINE;
                tk.line = lexer_->src_.line;
                tk.column = lexer_->src_.column;
                token_push(tokens, tk);
                lexer_->src_.line++;
                lexer_->src_.column = 0;
                break;
            }

            if (cur == '\0') {
                tk.type = TK_EOF;
                tk.line = lexer_->src_.line;
                tk.column = lexer_->src_.column;
                token_push(tokens, tk);
                lexer_->state = LSTATE_END;
                break;
            }

            if (cur == '(' || cur == ')' || cur == '[' || cur == ']'
                || cur == '{' || cur == '}' || cur == '+' || cur == '-' || cur == '*'
                || cur == '/') {
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
                default:  lexer_->state = LSTATE_ERROR; continue;
                }
                token_push(tokens, tk);
                break;
            }

            if (cur == ';') {
                lexer_->state = LSTATE_COMMENT;
                break;
            }

            if (isalpha(cur) || cur == '_') {
                pos = 0;
                lexer_->readnow[pos++] = cur;
                sline = lexer_->src_.line;
                scol = lexer_->src_.column - 1;
                lexer_->state = LSTATE_IDENT;
                break;
            }

            if (isdigit(cur)) {
                pos = 0;
                lexer_->readnow[pos++] = cur;
                sline = lexer_->src_.line;
                scol = lexer_->src_.column - 1;
                lexer_->state = LSTATE_ILITER;
                break;
            }

            if (cur == '"') {
                pos = 0;
                sline = lexer_->src_.line;
                scol = lexer_->src_.column - 1;
                lexer_->state = LSTATE_SLITER;
                break;
            }

            if (cur == '\'') {
                sline = lexer_->src_.line;
                scol = lexer_->src_.column - 1;
                lexer_->state = LSTATE_CLITER;
                break;
            }

            lexer_->state = LSTATE_ERROR;
            break;

        case LSTATE_IDENT:
            cur = *(lexer_->src_.raw++);
            lexer_->src_.column++;
            if (isalnum(cur) || cur == '_') {
                if (pos < 255)
                    lexer_->readnow[pos++] = cur;
                break;
            }
            lexer_->src_.raw--;
            lexer_->src_.column--;
            lexer_->readnow[pos] = '\0';
            tk.type = TK_IDENT;
            tk.line = sline;
            tk.column = scol;
            tk.value.string = strdup(lexer_->readnow);
            if (!tk.value.string)
                abort();
            token_push(tokens, tk);
            lexer_->state = LSTATE_NORMAL;
            break;

        case LSTATE_ILITER:
            cur = *(lexer_->src_.raw++);
            lexer_->src_.column++;
            if (isdigit(cur)) {
                if (pos < 255)
                    lexer_->readnow[pos++] = cur;
                break;
            }
            if (cur == '.') {
                if (pos < 255)
                    lexer_->readnow[pos++] = cur;
                lexer_->state = LSTATE_FLITER;
                break;
            }
            lexer_->src_.raw--;
            lexer_->src_.column--;
            lexer_->readnow[pos] = '\0';
            tk.type = TK_ILITER;
            tk.line = sline;
            tk.column = scol;
            tk.value.integer = (int64_t)strtoll(lexer_->readnow, NULL, 10);
            token_push(tokens, tk);
            lexer_->state = LSTATE_NORMAL;
            break;

        case LSTATE_FLITER:
            cur = *(lexer_->src_.raw++);
            lexer_->src_.column++;
            if (isdigit(cur)) {
                if (pos < 255)
                    lexer_->readnow[pos++] = cur;
                break;
            }
            if (cur == '.' && memchr(lexer_->readnow, '.', (size_t)pos) != NULL) {
                snprintf(dg.whaterr, sizeof(dg.whaterr),
                         "duplicate dot in float literal");
                dg.line = sline;
                dg.column = scol;
                dg.level_ = LEVEL_ERROR;
                diag_push(diags, dg);
                lexer_->readnow[pos] = '\0';
                tk.type = TK_FLITER;
                tk.line = sline;
                tk.column = scol;
                tk.value.float_ = strtod(lexer_->readnow, NULL);
                token_push(tokens, tk);
                while (*(lexer_->src_.raw) && *(lexer_->src_.raw) != ' '
                       && *(lexer_->src_.raw) != '\t' && *(lexer_->src_.raw) != '\n'
                       && *(lexer_->src_.raw) != '\r') {
                    lexer_->src_.raw++;
                    lexer_->src_.column++;
                }
                lexer_->state = LSTATE_NORMAL;
                break;
            }
            if (pos > 0 && lexer_->readnow[pos - 1] == '.') {
                snprintf(dg.whaterr, sizeof(dg.whaterr),
                         "unexpected character '%c' (0x%02X) in float literal",
                         cur, (unsigned char)cur);
                dg.line = sline;
                dg.column = scol;
                dg.level_ = LEVEL_ERROR;
                diag_push(diags, dg);
                lexer_->readnow[pos] = '\0';
                tk.type = TK_FLITER;
                tk.line = sline;
                tk.column = scol;
                tk.value.float_ = strtod(lexer_->readnow, NULL);
                token_push(tokens, tk);
                if (cur == '\n') {
                    lexer_->src_.line++;
                    lexer_->src_.column = 0;
                }
                lexer_->state = LSTATE_NORMAL;
                break;
            }
            lexer_->src_.raw--;
            lexer_->src_.column--;
            lexer_->readnow[pos] = '\0';
            tk.type = TK_FLITER;
            tk.line = sline;
            tk.column = scol;
            tk.value.float_ = strtod(lexer_->readnow, NULL);
            token_push(tokens, tk);
            lexer_->state = LSTATE_NORMAL;
            break;

        case LSTATE_SLITER:
            cur = *(lexer_->src_.raw++);
            lexer_->src_.column++;
            if (cur == '\0' || cur == '\n') {
                snprintf(dg.whaterr, sizeof(dg.whaterr),
                         "unterminated string literal");
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
                lexer_->state = LSTATE_NORMAL;
                break;
            }
            if (cur == '"') {
                lexer_->readnow[pos] = '\0';
                tk.type = TK_SLITER;
                tk.line = sline;
                tk.column = scol;
                tk.value.string = strdup(lexer_->readnow);
                if (!tk.value.string)
                    abort();
                token_push(tokens, tk);
                lexer_->state = LSTATE_NORMAL;
                break;
            }
            if (cur == '\\') {
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
                break;
            }
            if (pos < 255)
                lexer_->readnow[pos++] = cur;
            break;

        case LSTATE_CLITER:
            cur = *(lexer_->src_.raw++);
            lexer_->src_.column++;
            if (cur == '\0' || cur == '\n') {
                snprintf(dg.whaterr, sizeof(dg.whaterr),
                         "unterminated character literal");
                dg.line = sline;
                dg.column = scol;
                dg.level_ = LEVEL_ERROR;
                diag_push(diags, dg);
                lexer_->state = LSTATE_NORMAL;
                break;
            }
            if (cur == '\'') {
                snprintf(dg.whaterr, sizeof(dg.whaterr),
                         "empty character literal");
                dg.line = sline;
                dg.column = scol;
                dg.level_ = LEVEL_ERROR;
                diag_push(diags, dg);
                tk.type = TK_CLITER;
                tk.line = sline;
                tk.column = scol;
                tk.value.char_ = '\0';
                token_push(tokens, tk);
                lexer_->state = LSTATE_NORMAL;
                break;
            }
            if (cur == '\\') {
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
            }
            lexer_->readnow[0] = cur;
            cur = *(lexer_->src_.raw++);
            lexer_->src_.column++;
            if (cur != '\'') {
                snprintf(dg.whaterr, sizeof(dg.whaterr),
                         "unterminated character literal");
                dg.line = sline;
                dg.column = scol;
                dg.level_ = LEVEL_ERROR;
                diag_push(diags, dg);
                lexer_->state = LSTATE_NORMAL;
                break;
            }
            tk.type = TK_CLITER;
            tk.line = sline;
            tk.column = scol;
            tk.value.char_ = lexer_->readnow[0];
            token_push(tokens, tk);
            lexer_->state = LSTATE_NORMAL;
            break;

        case LSTATE_COMMENT:
            cur = *(lexer_->src_.raw++);
            lexer_->src_.column++;
            if (cur == '\0') {
                tk.type = TK_EOF;
                tk.line = lexer_->src_.line;
                tk.column = lexer_->src_.column;
                token_push(tokens, tk);
                lexer_->state = LSTATE_END;
                break;
            }
            if (cur == '\n') {
                tk.type = TK_NEXTLINE;
                tk.line = lexer_->src_.line;
                tk.column = lexer_->src_.column;
                token_push(tokens, tk);
                lexer_->src_.line++;
                lexer_->src_.column = 0;
                lexer_->state = LSTATE_NORMAL;
            }
            break;

        case LSTATE_ERROR:
            snprintf(dg.whaterr, sizeof(dg.whaterr),
                     "unexpected character '%c' (0x%02X)", cur, (unsigned char)cur);
            dg.line = sline;
            dg.column = scol;
            dg.level_ = LEVEL_ERROR;
            diag_push(diags, dg);
            lexer_->state = LSTATE_NORMAL;
            break;

        case LSTATE_END:
            break;
        }
    }
}
