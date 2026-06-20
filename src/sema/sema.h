/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                                     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * sema.h — Semantic analysis pass interface.
 *
 * sema_check() walks the AST after parsing, builds symbol tables, checks
 * for undeclared variables, infers types, and writes diagnostics.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifndef SEMA_SEMA_H
#define SEMA_SEMA_H

#include "ast/ast.h"
#include "diag/diag.h"
#include "type/type.h"

/*
 * Per-node semantic annotation.  Indexed by AST node index.
 */
typedef struct {
    type_tag type;
    int decl_idx;   /* IDENT: AST index of declaration node, else -1 */
} sema_annot;

DECLARE_VECTOR(sema_annot, sema)

/*
 * Symbol kinds for the tagged union.
 */
typedef enum {
    SYM_VAR,
    SYM_CONST,   /* compile-time constant */
    SYM_FUNC,
    SYM_TYPE,
} sym_kind;

/*
 * Variable symbol: local/global variable declaration.
 */
typedef struct {
    bool is_assigned;
    bool is_used;
    type_tag type;
    int ast_idx;    /* AST node index of this declaration */
} sym_var;

/*
 * Function symbol: function declaration/definition.
 */
typedef struct {
    bool is_used;
    type_tag ret_type;
    type_tag_vector param_types;
} sym_func;

/*
 * Constant symbol: compile-time constant declaration.
 */
typedef struct {
    bool is_used;
    type_tag type;
    int64_t value;
    int ast_idx;    /* AST node index of the ANODE_CONSTDECL */
} sym_const;

/*
 * Type symbol: named type alias (user-defined type).
 */
typedef struct {
    type_tag type;
} sym_type;

/*
 * Symbol table entry — tagged union keyed by name.
 *
 * decl_depth tracks which scope level this symbol was originally declared in.
 * When deep-copied into a nested scope, inherited entries keep their original
 * depth so they can be distinguished from entries declared locally.
 */
typedef struct {
    const char *key;
    sym_kind kind;
    int decl_depth;
    union {
        sym_var var;
        sym_const const_;
        sym_func func;
        sym_type type;
    };
} sym_entry;

/*
 * Run semantic analysis over the AST.  Appends diagnostics to *diags.
 */
sema_vector sema_check(node_vector nodes, diag_vector *diags);

#endif /* SEMA_SEMA_H */
