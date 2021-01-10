project('lisp', 'cpp')

executable(
  'interpreter',
  sources: [
    'src/main.cpp',
    'src/yl/parse.cpp',
    'src/yl/eval.cpp',
    'src/yl/types.cpp',
  ],
  include_directories: [
    'include'
  ],
  override_options: [
    'cpp_std=c++17',
  ],
  dependencies: [
    dependency('readline'),
  ],  
)
