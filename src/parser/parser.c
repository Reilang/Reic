/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * parser.c — State-machine parser driven by a state stack.
 *
 * Reads a flat token_vector via cursor, emits AST nodes into the caller-supplied
 * node_vector.  The state stack drives grammar recognition; each state
 * corresponds to a syntactic construct currently being parsed.
 *
 * Empty state stack = file level.  PSTATE_BLOCK tracks brace nesting so '}'
 * is handled at the right scope.  All other constructs are parsed synchronously
 * by dedicated functions (parse_funcdef, parse_return, etc.).
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "parser/parser.h"
#include "type/type.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static token curtok(const parser *p)
{
    if (p->cursor >= p->tokens.size) {
        token eof = {0};
        eof.type = TK_EOF;
        return eof;
    }
    return token_vec_get(&p->tokens, p->cursor);
}

static int at_eof(const parser *p)
{
    return p->cursor >= p->tokens.size
        || token_vec_get(&p->tokens, p->cursor).type == TK_EOF;
}

static void skip_newlines(parser *p)
{
    while (p->cursor < p->tokens.size
           && token_vec_get(&p->tokens, p->cursor).type == TK_NEXTLINE)
        p->cursor++;
}

static int new_node(node_vector *nodes, anode_kind kind)
{
    anode n = {0};
    n.kind = kind;
    n.child = -1;
    n.next = -1;
    node_vec_push(nodes, n);
    return nodes->size - 1;
}

static int new_ident(node_vector *nodes, const char *name, anode_kind kind)
{
    int idx = new_node(nodes, kind);
    nodes->data[idx].sv = strdup(name);
    return idx;
}

/* stupid forward declaration */
static int parse_stmt(parser *p, node_vector *nodes, diag_vector *diags);
static int parse_expr(parser *p, node_vector *nodes, diag_vector *diags, int min_prec);
static int parse_funcdef(parser *p, node_vector *nodes, diag_vector *diags);
static int parse_vardecl(parser *p, node_vector *nodes, diag_vector *diags);
static int parse_assign(parser *p, node_vector *nodes, diag_vector *diags);
static int parse_if(parser *p, node_vector *nodes, diag_vector *diags);
static int parse_while(parser *p, node_vector *nodes, diag_vector *diags);
static int parse_loop(parser *p, node_vector *nodes, diag_vector *diags);
static int parse_return(parser *p, node_vector *nodes, diag_vector *diags);

/*
 * We need to sync it. You don't want tons of errors, do you?
 */
static void sync(parser *p)
{
    while (p->cursor < p->tokens.size) {
        tktype t = token_vec_get(&p->tokens, p->cursor).type;
        if (t == TK_NEXTLINE || t == TK_CBRACE || t == TK_OBRACE || t == TK_EOF)
            return;
        p->cursor++;
    }
}

static int parse_stmt(parser *p, node_vector *nodes, diag_vector *diags)
{
    token tk = curtok(p);

    if (tk.type == TK_KEYWORD) {
        if (strcmp(tk.value.string, "fn") == 0)
            return parse_funcdef(p, nodes, diags);
        if (strcmp(tk.value.string, "var") == 0)
            return parse_vardecl(p, nodes, diags);
        if (strcmp(tk.value.string, "if") == 0)
            return parse_if(p, nodes, diags);
        if (strcmp(tk.value.string, "while") == 0)
            return parse_while(p, nodes, diags);
        if (strcmp(tk.value.string, "loop") == 0)
            return parse_loop(p, nodes, diags);
        if (strcmp(tk.value.string, "return") == 0)
            return parse_return(p, nodes, diags);
    }

    if (tk.type == TK_IDENT)
        return parse_assign(p, nodes, diags);

    diag_add(diags, LEVEL_ERROR, "expected statement",
            tk.line, tk.column);
    sync(p);
    return -1;
}

static int parse_funcdef(parser *p, node_vector *nodes, diag_vector *diags)
{
    token tk;
    int name_idx, rettype_idx, func_idx;
    int param_head = -1;
    int param_tail = -1;
    int prev_link;
    int initial_sz;

    p->cursor++; /* skip 'fn' */

    skip_newlines(p);
    tk = curtok(p);
    if (tk.type != TK_IDENT) {
        diag_add(diags, LEVEL_ERROR, "expected function name after 'fn'",
                tk.line, tk.column);
        sync(p);
        return -1;
    }
    name_idx = new_ident(nodes, tk.value.string, ANODE_IDENT_FUNC);
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_OPAREN) {
        diag_add(diags, LEVEL_ERROR, "expected '(' after function name",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;

    while (curtok(p).type != TK_CPAREN) {
        int pname_idx, ptype_idx, vardecl_idx;

        if (at_eof(p)) {
            diag_add(diags, LEVEL_ERROR, "expected ')' to close parameter list",
                     tk.line, tk.column);
            sync(p);
            return -1;
        }

        tk = curtok(p);
        if (tk.type != TK_IDENT) {
            diag_add(diags, LEVEL_ERROR, "expected parameter name",
                    tk.line, tk.column);
            sync(p);
            return -1;
        }
        pname_idx = new_ident(nodes, tk.value.string, ANODE_IDENT_VAR);
        p->cursor++;

        if (curtok(p).type != TK_COLON) {
            diag_add(diags, LEVEL_ERROR, "expected ':' after parameter name",
                     curtok(p).line, curtok(p).column);
            sync(p);
            return -1;
        }
        p->cursor++;

        tk = curtok(p);
        if (tk.type != TK_IDENT) {
            diag_add(diags, LEVEL_ERROR, "expected type name",
                    tk.line, tk.column);
            sync(p);
            return -1;
        }
        {
            type_tag tag = type_from_name(tk.value.string);
            if (tag == TYPE_COUNT) {
                diag_add(diags, LEVEL_ERROR, "unknown type name",
                        tk.line, tk.column);
                sync(p);
                return -1;
            }
            ptype_idx = new_node(nodes, ANODE_IDENT_TYPE);
            nodes->data[ptype_idx].iv = tag;
        }
        p->cursor++;

        vardecl_idx = new_node(nodes, ANODE_VARDECL);
        nodes->data[vardecl_idx].child = pname_idx;
        nodes->data[pname_idx].next = ptype_idx;

        if (param_head < 0)
            param_head = vardecl_idx;
        else
            nodes->data[param_tail].next = vardecl_idx;
        param_tail = vardecl_idx;

        if (curtok(p).type == TK_COMMA) {
            p->cursor++;
        } else if (curtok(p).type != TK_CPAREN) {
            diag_add(diags, LEVEL_ERROR, "expected ',' or ')' after parameter",
                     curtok(p).line, curtok(p).column);
            sync(p);
            return -1;
        }
    }
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_MINUS) {
        diag_add(diags, LEVEL_ERROR, "expected '->' after parameters",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;
    skip_newlines(p);
    if (curtok(p).type != TK_CABRACKET) {
        diag_add(diags, LEVEL_ERROR, "expected '->' (missing '>' after '-')",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;

    skip_newlines(p);
    tk = curtok(p);
    if (tk.type != TK_IDENT) {
        diag_add(diags, LEVEL_ERROR, "expected return type after '->'",
                tk.line, tk.column);
        sync(p);
        return -1;
    }
    {
        type_tag tag = type_from_name(tk.value.string);
        if (tag == TYPE_COUNT) {
            diag_add(diags, LEVEL_ERROR, "unknown type name",
                    tk.line, tk.column);
            sync(p);
            return -1;
        }
        rettype_idx = new_node(nodes, ANODE_IDENT_TYPE);
        nodes->data[rettype_idx].iv = tag;
    }
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_OBRACE) {
        diag_add(diags, LEVEL_ERROR, "expected '{' before function body",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;

    state_vec_push(&p->states, PSTATE_BLOCK);
    initial_sz = p->states.size;
    {
        int block_idx = new_node(nodes, ANODE_BLOCK);
        int first_stmt = -1;
        int last_stmt = -1;

        while (p->states.size >= initial_sz && p->cursor < p->tokens.size) {
            skip_newlines(p);
            if (at_eof(p)) {
                diag_add(diags, LEVEL_ERROR, "unexpected end of file in function body",
                         curtok(p).line, curtok(p).column);
                break;
            }

            if (state_vec_get(&p->states, p->states.size - 1) == PSTATE_BLOCK) {
                tk = curtok(p);
                if (tk.type == TK_CBRACE) {
                    p->cursor++;
                    state_vec_pop(&p->states);
                    continue;
                }
                if (tk.type == TK_OBRACE) {
                    p->cursor++;
                    state_vec_push(&p->states, PSTATE_BLOCK);
                    continue;
                }
                {
                    int sidx = parse_stmt(p, nodes, diags);
                    if (sidx >= 0) {
                        if (first_stmt < 0)
                            first_stmt = sidx;
                        else
                            nodes->data[last_stmt].next = sidx;
                        last_stmt = sidx;
                    }
                }
            }
        }

        nodes->data[block_idx].child = first_stmt;

        func_idx = new_node(nodes, ANODE_FUNCDECL);
        nodes->data[func_idx].child = name_idx;

        prev_link = name_idx;
        if (param_head >= 0) {
            nodes->data[prev_link].next = param_head;
            prev_link = param_head;
            while (nodes->data[prev_link].next >= 0)
                prev_link = nodes->data[prev_link].next;
        }
        nodes->data[prev_link].next = rettype_idx;
        nodes->data[rettype_idx].next = block_idx;
    }

    return func_idx;
}

static int parse_vardecl(parser *p, node_vector *nodes, diag_vector *diags)
{
    token tk;
    int name_idx, type_idx, init_idx, vardecl_idx;
    int prev;

    p->cursor++; /* skip 'var' */

    skip_newlines(p);
    tk = curtok(p);
    if (tk.type != TK_IDENT) {
        diag_add(diags, LEVEL_ERROR, "expected variable name after 'var'",
                tk.line, tk.column);
        sync(p);
        return -1;
    }

    name_idx = new_ident(nodes, tk.value.string, ANODE_IDENT_VAR);
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_COLON) {
        diag_add(diags, LEVEL_ERROR, "expected ':' after variable name",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;
    skip_newlines(p);

    type_idx = -1;
    init_idx = -1;

    if (curtok(p).type == TK_EQUAL) {
        /* var x := expr */
        p->cursor++;
        skip_newlines(p);
        init_idx = parse_expr(p, nodes, diags, 0);
        if (init_idx < 0) return -1;
    } else if (curtok(p).type == TK_IDENT) {
        /* var x: type */
        tk = curtok(p);
        {
            type_tag tag = type_from_name(tk.value.string);
            if (tag == TYPE_COUNT) {
                diag_add(diags, LEVEL_ERROR, "unknown type name",
                        tk.line, tk.column);
                sync(p);
                return -1;
            }
            type_idx = new_node(nodes, ANODE_IDENT_TYPE);
            nodes->data[type_idx].iv = tag;
        }
        p->cursor++;

        skip_newlines(p);
        /* optionally: `:= expr` */
        if (curtok(p).type == TK_COLON) {
            p->cursor++;
            skip_newlines(p);
            if (curtok(p).type != TK_EQUAL) {
                diag_add(diags, LEVEL_ERROR, "expected ':=' after type",
                         curtok(p).line, curtok(p).column);
                sync(p);
                return -1;
            }
            p->cursor++;
            skip_newlines(p);
            init_idx = parse_expr(p, nodes, diags, 0);
            if (init_idx < 0) return -1;
        }
    } else {
        diag_add(diags, LEVEL_ERROR, "expected type name or ':='",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }

    vardecl_idx = new_node(nodes, ANODE_VARDECL);
    nodes->data[vardecl_idx].child = name_idx;

    prev = name_idx;
    if (type_idx >= 0) {
        nodes->data[prev].next = type_idx;
        prev = type_idx;
    }
    if (init_idx >= 0) {
        nodes->data[prev].next = init_idx;
    }

    return vardecl_idx;
}

static int parse_assign(parser *p, node_vector *nodes, diag_vector *diags)
{
    token tk = curtok(p);
    int var_idx, expr_idx, assign_idx;


    var_idx = new_ident(nodes, tk.value.string, ANODE_IDENT_VAR);
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_COLON) {
        diag_add(diags, LEVEL_ERROR, "expected ':=' in assignment",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;
    skip_newlines(p);
    if (curtok(p).type != TK_EQUAL) {
        diag_add(diags, LEVEL_ERROR, "expected ':=' in assignment (missing '=' after ':')",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;
    skip_newlines(p);

    expr_idx = parse_expr(p, nodes, diags, 0);
    if (expr_idx < 0) return -1;

    assign_idx = new_node(nodes, ANODE_ASSIGN);
    nodes->data[assign_idx].child = var_idx;
    nodes->data[var_idx].next = expr_idx;

    return assign_idx;
}

static int parse_expr(parser *p, node_vector *nodes, diag_vector *diags, int min_prec);
static int parse_primary(parser *p, node_vector *nodes, diag_vector *diags);

static int parse_primary(parser *p, node_vector *nodes, diag_vector *diags)
{
    token tk = curtok(p);

    if (tk.type == TK_ILITER) {
        int idx = new_node(nodes, ANODE_ILITERAL);
        nodes->data[idx].iv = tk.value.integer;
        p->cursor++;
        return idx;
    }
    if (tk.type == TK_FLITER) {
        int idx = new_node(nodes, ANODE_FLITERAL);
        nodes->data[idx].fv = tk.value.float_;
        p->cursor++;
        return idx;
    }
    if (tk.type == TK_IDENT) {
        int idx = new_ident(nodes, tk.value.string, ANODE_IDENT);
        p->cursor++;
        return idx;
    }
    if (tk.type == TK_OPAREN) {
        p->cursor++;
        skip_newlines(p);
        int idx = parse_expr(p, nodes, diags, 0);
        skip_newlines(p);
        if (curtok(p).type != TK_CPAREN) {
            diag_add(diags, LEVEL_ERROR, "expected ')'",
                     curtok(p).line, curtok(p).column);
            sync(p);
            return idx >= 0 ? idx : -1;
        }
        p->cursor++;
        return idx;
    }

    diag_add(diags, LEVEL_ERROR, "expected expression",
             tk.line, tk.column);
    sync(p);
    return -1;
}

static int parse_expr(parser *p, node_vector *nodes, diag_vector *diags, int min_prec)
{
    int left = parse_primary(p, nodes, diags);
    if (left < 0)
        return -1;

    for (;;) {
        token tk = curtok(p);
        int prec;

        switch (tk.type) {
        case TK_OABRACKET:
        case TK_CABRACKET: prec = 1; break;
        case TK_ADD:
        case TK_MINUS:     prec = 2; break;
        case TK_STAR:
        case TK_SLASH:     prec = 3; break;
        default:           prec = 0; break;
        }
        if (prec <= min_prec)
            break;

        tktype op = tk.type;
        p->cursor++;

        int right = parse_expr(p, nodes, diags, prec);
        if (right < 0)
            return left;

        int binop = new_node(nodes, ANODE_BINOP);
        nodes->data[binop].op = op;
        nodes->data[binop].child = left;
        nodes->data[left].next = right;
        left = binop;
    }

    return left;
}

static int parse_return(parser *p, node_vector *nodes, diag_vector *diags)
{
    p->cursor++;
    int ret_idx = new_node(nodes, ANODE_RETURN);
    int expr_idx = parse_expr(p, nodes, diags, 0);
    if (expr_idx >= 0)
        nodes->data[ret_idx].child = expr_idx;
    return ret_idx;
}

static int parse_if(parser *p, node_vector *nodes, diag_vector *diags)
{
    token tk;
    int scrutinee_idx, if_idx;
    int arm_head = -1;
    int arm_tail = -1;

    p->cursor++; /* skip 'if' */

    skip_newlines(p);
    if (curtok(p).type != TK_OPAREN) {
        diag_add(diags, LEVEL_ERROR, "expected '(' after 'if'",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;

    scrutinee_idx = parse_expr(p, nodes, diags, 0);
    if (scrutinee_idx < 0) return -1;

    if (curtok(p).type != TK_CPAREN) {
        diag_add(diags, LEVEL_ERROR, "expected ')' after if condition",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_OBRACE) {
        diag_add(diags, LEVEL_ERROR, "expected '{' before if body",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;

    while (curtok(p).type != TK_CBRACE && !at_eof(p)) {
        int pattern_idx, arm_idx;
        int body_head = -1;
        int body_tail = -1;

        skip_newlines(p);
        tk = curtok(p);

        if (tk.type == TK_CBRACE)
            break;

        if (tk.type != TK_EQUAL) {
            diag_add(diags, LEVEL_ERROR, "expected '=' to start match arm",
                     tk.line, tk.column);
            sync(p);
            if (arm_head < 0) return -1;
            break;
        }
        p->cursor++;

        skip_newlines(p);
        pattern_idx = parse_expr(p, nodes, diags, 0);
        if (pattern_idx < 0) return -1;

        skip_newlines(p);
        /* '=>' */
        if (curtok(p).type != TK_EQUAL) {
            diag_add(diags, LEVEL_ERROR, "expected '=>' after match pattern",
                     curtok(p).line, curtok(p).column);
            sync(p);
            return -1;
        }
        p->cursor++;
        skip_newlines(p);
        if (curtok(p).type != TK_CABRACKET) {
            diag_add(diags, LEVEL_ERROR,
                     "expected '=>' (missing '>' after '=')",
                     curtok(p).line, curtok(p).column);
            sync(p);
            return -1;
        }
        p->cursor++;

        /* arm body: stmts until next '=' or '}' */
        for (;;) {
            skip_newlines(p);
            tk = curtok(p);
            if (tk.type == TK_EQUAL || tk.type == TK_CBRACE || at_eof(p))
                break;

            int sidx = parse_stmt(p, nodes, diags);
            if (sidx >= 0) {
                if (body_head < 0)
                    body_head = sidx;
                else
                    nodes->data[body_tail].next = sidx;
                body_tail = sidx;
            }
        }

        arm_idx = new_node(nodes, ANODE_MATCHARM);
        nodes->data[arm_idx].child = pattern_idx;
        if (body_head >= 0)
            nodes->data[pattern_idx].next = body_head;

        if (arm_head < 0)
            arm_head = arm_idx;
        else
            nodes->data[arm_tail].next = arm_idx;
        arm_tail = arm_idx;
    }

    if (curtok(p).type == TK_CBRACE)
        p->cursor++;

    if_idx = new_node(nodes, ANODE_IF);
    nodes->data[if_idx].child = scrutinee_idx;
    if (arm_head >= 0)
        nodes->data[scrutinee_idx].next = arm_head;

    return if_idx;
}

static int parse_while(parser *p, node_vector *nodes, diag_vector *diags)
{
    int cond_idx, block_idx, while_idx;
    int first_stmt = -1;
    int last_stmt = -1;
    int initial_sz;

    p->cursor++; /* skip 'while' */

    skip_newlines(p);
    if (curtok(p).type != TK_OPAREN) {
        diag_add(diags, LEVEL_ERROR, "expected '(' after 'while'",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;

    cond_idx = parse_expr(p, nodes, diags, 0);
    if (cond_idx < 0) return -1;

    if (curtok(p).type != TK_CPAREN) {
        diag_add(diags, LEVEL_ERROR, "expected ')' after while condition",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;

    skip_newlines(p);
    if (curtok(p).type != TK_OBRACE) {
        diag_add(diags, LEVEL_ERROR, "expected '{' before while body",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;

    state_vec_push(&p->states, PSTATE_BLOCK);
    initial_sz = p->states.size;
    {
        block_idx = new_node(nodes, ANODE_BLOCK);

        while (p->states.size >= initial_sz && p->cursor < p->tokens.size) {
            skip_newlines(p);
            if (at_eof(p)) {
                diag_add(diags, LEVEL_ERROR,
                         "unexpected end of file in while body",
                         curtok(p).line, curtok(p).column);
                break;
            }

            if (state_vec_get(&p->states, p->states.size - 1) == PSTATE_BLOCK) {
                token tk = curtok(p);
                if (tk.type == TK_CBRACE) {
                    p->cursor++;
                    state_vec_pop(&p->states);
                    continue;
                }
                if (tk.type == TK_OBRACE) {
                    p->cursor++;
                    state_vec_push(&p->states, PSTATE_BLOCK);
                    continue;
                }
                {
                    int sidx = parse_stmt(p, nodes, diags);
                    if (sidx >= 0) {
                        if (first_stmt < 0)
                            first_stmt = sidx;
                        else
                            nodes->data[last_stmt].next = sidx;
                        last_stmt = sidx;
                    }
                }
            }
        }

        nodes->data[block_idx].child = first_stmt;
    }

    while_idx = new_node(nodes, ANODE_WHILE);
    nodes->data[while_idx].child = cond_idx;
    nodes->data[cond_idx].next = block_idx;

    return while_idx;
}

static int parse_loop(parser *p, node_vector *nodes, diag_vector *diags)
{
    int block_idx, loop_idx;
    int first_stmt = -1;
    int last_stmt = -1;
    int initial_sz;

    p->cursor++; /* skip 'loop' */

    skip_newlines(p);
    if (curtok(p).type != TK_OBRACE) {
        diag_add(diags, LEVEL_ERROR, "expected '{' after 'loop'",
                 curtok(p).line, curtok(p).column);
        sync(p);
        return -1;
    }
    p->cursor++;

    state_vec_push(&p->states, PSTATE_BLOCK);
    initial_sz = p->states.size;
    {
        block_idx = new_node(nodes, ANODE_BLOCK);

        while (p->states.size >= initial_sz && p->cursor < p->tokens.size) {
            skip_newlines(p);
            if (at_eof(p)) {
                diag_add(diags, LEVEL_ERROR,
                         "unexpected end of file in loop body",
                         curtok(p).line, curtok(p).column);
                break;
            }

            if (state_vec_get(&p->states, p->states.size - 1) == PSTATE_BLOCK) {
                token tk = curtok(p);
                if (tk.type == TK_CBRACE) {
                    p->cursor++;
                    state_vec_pop(&p->states);
                    continue;
                }
                if (tk.type == TK_OBRACE) {
                    p->cursor++;
                    state_vec_push(&p->states, PSTATE_BLOCK);
                    continue;
                }
                {
                    int sidx = parse_stmt(p, nodes, diags);
                    if (sidx >= 0) {
                        if (first_stmt < 0)
                            first_stmt = sidx;
                        else
                            nodes->data[last_stmt].next = sidx;
                        last_stmt = sidx;
                    }
                }
            }
        }

        nodes->data[block_idx].child = first_stmt;
    }

    loop_idx = new_node(nodes, ANODE_LOOP);
    nodes->data[loop_idx].child = block_idx;

    return loop_idx;
}

void parse(parser *p, node_vector *nodes, diag_vector *diags)
{
    while (p->cursor < p->tokens.size) {
        skip_newlines(p);
        if (at_eof(p))
            break;

        if (p->states.size == 0) {
            token tk = curtok(p);
            if (tk.type == TK_CBRACE) {
                diag_add(diags, LEVEL_ERROR, "unexpected '}'",
                        tk.line, tk.column);
                p->cursor++;
            } else if (tk.type == TK_OBRACE) {
                p->cursor++;
                state_vec_push(&p->states, PSTATE_BLOCK);
            } else {
                parse_stmt(p, nodes, diags);
            }
        } else {
            switch (state_vec_get(&p->states, p->states.size - 1)) {
            case PSTATE_BLOCK: {
                token tk = curtok(p);
                if (tk.type == TK_CBRACE) {
                    p->cursor++;
                    state_vec_pop(&p->states);
                } else if (tk.type == TK_OBRACE) {
                    p->cursor++;
                    state_vec_push(&p->states, PSTATE_BLOCK);
                } else {
                    parse_stmt(p, nodes, diags);
                }
                break;
            }
            default:
                break;
            }
        }
    }
}
