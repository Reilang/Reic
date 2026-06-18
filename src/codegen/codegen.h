/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * codegen.h — LLVM IR text emitter interface.
 *
 * codegen_emit() walks the AST and writes a .ll file.  No LLVM libraries
 * are needed — we just fprintf the human-readable IR text, then let clang
 * compile it.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifndef CODEGEN_CODEGEN_H
#define CODEGEN_CODEGEN_H

#include "ast/ast.h"

/*
 * Walks the AST and emits human-readable LLVM IR to output_path.
 * Returns 0 on success, non-zero on failure (e.g. cannot open file).
 * Only handles FUNCDECL, var decls, assignments, and return expressions.
 */
int codegen_emit(node_vector *nodes, const char *output_path);

#endif /* CODEGEN_CODEGEN_H */
