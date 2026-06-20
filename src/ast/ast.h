/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * ast.h — AST node kind enum and the anode struct.
 *
 * All nodes live in a flat node_vector (child/next are indices, not pointers).
 * Variadic children (block stmts, call args, func params) use sibling chains:
 * parent.child -> first child, .next -> sibling, .next -> sibling, ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifndef AST_AST_H
#define AST_AST_H

#include "collect/vector.h"
#include "token/token.h"

/* Every syntactic construct the parser can produce. */
typedef enum {
    ANODE_NONE,

    /* identifiers (semantic role chosen by parser context) */
    ANODE_IDENT,        /* unresolved reference (expression context) */
    ANODE_IDENT_FUNC,   /* function name */
    ANODE_IDENT_VAR,    /* variable name (declaration site) */
    ANODE_IDENT_TYPE,   /* type name (resolved to type_tag in .iv) */

    /* literals */
    ANODE_ILITERAL,     /* integer: value in .iv */
    ANODE_FLITERAL,     /* float:   value in .fv */
    ANODE_SLITERAL,     /* string:  value in .sv */
    ANODE_CLITERAL,     /* char:    value in .cv */

    /* expressions */
    ANODE_BINOP,        /* binary operator: .op holds tktype */
    ANODE_UNOP,         /* unary operator:  .op holds tktype */
    ANODE_ASSIGN,       /* assignment: child=target, target.next=rhs */
    ANODE_CALL,         /* function call */
    ANODE_INDEX,        /* array/map index */

    /* statements */
    ANODE_BLOCK,        /* { } block: child=first stmt, stmts linked via .next */
    ANODE_IF,           /* if (expr) { arms } — scrutinee via child, arms via next */
    ANODE_WHILE,        /* while (cond) { body } — cond via child, body via next */
    ANODE_LOOP,         /* loop { body } — body via child */
    ANODE_FOR,          /* for in range — not yet implemented */
    ANODE_RETURN,       /* return [expr] — expr via child */
    ANODE_MATCHARM,     /* op pattern => body — op in .op, pattern via child, body via next */

    /* declarations */
    ANODE_FUNCDECL,     /* fn name(params) -> rettype { body } */
    ANODE_VARDECL,      /* var name[: type][:= init] */
    ANODE_CONSTDECL,    /* name = value  -- compile-time constant */
    ANODE_TYPEDECL,     /* type alias — not yet implemented */

    ANODE_COUNT
} anode_kind;

/*
 * A single AST node.  child/next are integer indices into the flat node_vector
 * (-1 means none).  The union field is interpreted according to .kind:
 *
 *   IDENT variants -> .sv  (allocated string)
 *   ILITERAL       -> .iv  (int64_t)
 *   FLITERAL       -> .fv  (double)
 *   SLITERAL       -> .sv  (allocated string)
 *   CLITERAL       -> .cv  (char)
 *   IDENT_TYPE     -> .iv  (type_tag enum value)
 *   BINOP / UNOP   -> .op  (tktype operator)
 */
typedef struct {
    anode_kind kind;
    int child;  /* index of first child, -1 if none */
    int next;   /* index of next sibling, -1 if none */
    union {
        int64_t iv;
        double fv;
        char cv;
        char *sv;
        tktype op;
    };
} anode;

DECLARE_VECTOR(anode, node)

/*
 * Allocates a one-line debug string for a single node.
 * Caller must free() the result.
 */
char *anode_print(anode node_);

/*
 * Builds an ASCII tree representation of the entire AST (pass by value:
 * node_vector is a small metadata struct, not mutated).
 * Caller must free() the result.
 */
char *ast_print_tree(node_vector nodes);

#endif /* AST_AST_H */
