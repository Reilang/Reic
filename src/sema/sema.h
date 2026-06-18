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

/*
 * Run semantic analysis over the AST.  Appends diagnostics to *diags.
 * Currently placeholder — framework in place, symbol table and checks TBD.
 */
void sema_check(node_vector nodes, diag_vector *diags);

#endif /* SEMA_SEMA_H */
