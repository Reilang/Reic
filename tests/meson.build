test_deps = [reic_lib]
test_inc = include_directories('../src', '.')

test_lexer_exe = executable('test_lexer',
  'test_lexer.c',
  link_with: test_deps,
  include_directories: test_inc,
)

test('lexer', test_lexer_exe)

test_parser_exe = executable('test_parser',
  'test_parser.c',
  link_with: test_deps,
  include_directories: test_inc,
)

test('parser', test_parser_exe)

test_sema_exe = executable('test_sema',
  'test_sema.c',
  link_with: test_deps,
  include_directories: test_inc,
)

test('sema', test_sema_exe)

test_codegen_exe = executable('test_codegen',
  'test_codegen.c',
  link_with: test_deps,
  include_directories: test_inc,
)

test('codegen', test_codegen_exe)
