project('lisp', 'cpp')

executable(
  'interpeter',
  sources: [
    'src/main.cpp',
    'src/yl/parse.cpp',
    'src/yl/eval.cpp',
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
