lib_srcs = files(
  'lexer/lexer.c',
  'token/token.c',
  'diag/diag.c',
  'type/type.c',
  'parser/parser.c',
  'ast/ast.c',
  'hir/hir.c',
  'hir/hir_lower.c',
  'sema/sema.c',
  'codegen/codegen.c',
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
