# Yatsukha's Lisp

 Lisp like interpreted language realized using C++. Being intrigued by the [Build Your Own Lisp](http://www.buildyourownlisp.com) guide, I started writting my own Lisp implementation. Although the guide is quite good for writting in pure C, I was usually just looking at the examples and thinking of how I can figure it out on my own, after all that's where all the fun lies.

## Features

This section just lists interesting features, not all of them.

 * code as value
 * predef loading
 * basic error reporting, it will also print the line where the error is if it was entered previously in the interpreter
 * variadic arguments for user defined functions
 * partially evaluated user defined functions: if the function is not variadic and you pass less than the required number of arguments the function will be bound to those arguments
 * breaking expressions into multiple lines

## Building

Requires:
  * ninja
  * meson
  * C++17 compiler
  * readline

In the directory where `meson.build` resides:

```
$ meson build
$ cd build
$ ninja
```

## Running

In the build directory:

```
$ ./interpreter
```

Use `help` from here to get going. Since the executable is in the build directory it will not detect the predef file that is in the root directory. To fix this just link it using `ln -s ../.predef.yl .`.

To interpret a file pass it as an argument to the interpreter.

```
$ ./interpreter ../examples.yl
```
