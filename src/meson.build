lib_srcs = files(
  'lexer/lexer.c',
  'token/token.c',
  'diag/diag.c',
  'type/type.c',
  'parser/parser.c',
  'parser/parser_decl.c',
  'parser/parser_stmt.c',
  'parser/parser_expr.c',
  'ast/ast.c',
  'hir/hir.c',
  'hir/hir_lower.c',
  'sema/sema.c',
  'sema/sema_decl.c',
  'sema/sema_stmt.c',
  'sema/sema_expr.c',
  'codegen/codegen.c',
  'codegen/codegen_expr.c',
  'codegen/codegen_stmt.c',
)

reic_lib = static_library('reic',
  lib_srcs,
  include_directories: include_directories('.'),
)

executable('reic',
  'main.c',
  link_with: reic_lib,
  include_directories: include_directories('.'),
  install: true,
)
