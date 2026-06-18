srcs = files(
  'main.c',
  'lexer/lexer.c',
  'token/token.c',
  'diag/diag.c',
  'type/type.c',
  'parser/parser.c',
  'ast/ast.c',
  'sema/sema.c',
  'codegen/codegen.c',
)

executable('reic',
  srcs,
  include_directories: include_directories('.'),
  install: true,
)
