project('reilang', 'c',
  version: '0.1.0',
  default_options: [
    'c_std=c11',
    'warning_level=3',
    'buildtype=debugoptimized',
  ],
  meson_version: '>=1.1.0',
)

cc = meson.get_compiler('c')

add_project_arguments(
  cc.get_supported_arguments([
    '-Wpedantic',
    '-Wstrict-prototypes',
    '-Wmissing-prototypes',
    '-Wshadow',
    '-Wpointer-arith',
    '-Wcast-qual',
    '-Wconversion',
  ]),
  language: 'c',
)

if get_option('asan')
  add_project_arguments(
    cc.get_supported_arguments(['-fsanitize=address', '-fsanitize=undefined']),
    language: 'c',
  )
  add_project_link_arguments(
    cc.get_supported_link_arguments(['-fsanitize=address', '-fsanitize=undefined']),
    language: 'c',
  )
endif

subdir('src')
