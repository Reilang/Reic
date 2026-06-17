/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
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

typedef enum {
    ANODE_NONE,

    ANODE_IDENT,
    ANODE_IDENT_FUNC,
    ANODE_IDENT_VAR,
    ANODE_IDENT_TYPE,
    ANODE_ILITERAL,
    ANODE_FLITERAL,
    ANODE_SLITERAL,
    ANODE_CLITERAL,

    ANODE_BINOP,
    ANODE_UNOP,
    ANODE_ASSIGN, 
    ANODE_CALL,  
    ANODE_INDEX,

    ANODE_BLOCK,
    ANODE_IF,
    ANODE_WHILE,
    ANODE_FOR,
    ANODE_RETURN,
    ANODE_MATCHARM,

    ANODE_FUNCDECL,
    ANODE_VARDECL,
    ANODE_TYPEDECL,

    ANODE_COUNT
} anode_kind;

typedef struct {
    anode_kind kind;
    int child;
    int next;
    union {
        int64_t iv;
        double fv;
        char cv;
        char *sv;
        tktype op;
    };
} anode;

DECLARE_VECTOR(anode, node)

char *anode_print(anode node_);
char *ast_print_tree(node_vector nodes);

#endif /* AST_AST_H */
