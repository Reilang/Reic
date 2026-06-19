/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * hir_lower.h — AST -> HIR lowering pass interface.
 *
 * Consumes the AST and its per-node semantic annotations, produces a typed HIR
 * with resolved identifier references and implicit casts.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifndef HIR_HIR_LOWER_H
#define HIR_HIR_LOWER_H

#include "ast/ast.h"
#include "hir/hir.h"
#include "sema/sema.h"

hir_vector hir_lower(node_vector nodes, const sema_vector *annot);

#endif /* HIR_HIR_LOWER_H */
