project('lisp', 'cpp')

executable(
  'interpreter',
  sources: [
    'src/main.cpp',
    'src/yl/parse.cpp',
    'src/yl/eval.cpp',
    'src/yl/types.cpp',
    'src/yl/user_io.cpp',
    'src/yl/history.cpp',
  ],
  include_directories: [
    'include',
    'immer',
  ],
  override_options: [
    'cpp_std=c++17',
  ],
  cpp_args: [
    '-march=native',
  ],
  dependencies: [
    dependency('readline'),
  ],
)
