/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * codegen.h — LLVM IR text emitter interface.
 *
 * codegen_emit() walks the HIR and writes a .ll file.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifndef CODEGEN_CODEGEN_H
#define CODEGEN_CODEGEN_H

#include "hir/hir.h"

/*
 * Walks the HIR and emits human-readable LLVM IR to output_path.
 * Returns 0 on success, non-zero on failure (e.g. cannot open file).
 */
int codegen_emit(hir_vector *hir, const char *output_path);

#endif /* CODEGEN_CODEGEN_H */
