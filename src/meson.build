srcs = files(
  'main.c',
  'lexer/lexer.c',
  'token/token.c',
  'diag/diag.c',
  'parser/parser.c',
  'ast/ast.c',
  'codegen/codegen.c',
)

executable('reilang',
  srcs,
  include_directories: include_directories('.'),
  install: true,
)
