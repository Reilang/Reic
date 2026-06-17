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

int codegen_emit(node_vector *nodes, const char *output_path);

#endif /* CODEGEN_CODEGEN_H */
