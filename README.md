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
 * recursive functions

## Building

Requires:
  * ninja
  * meson
  * C++17 compiler
  * readline

It is recommended to build as for release if you plan on running more complex programs.
In the directory where `meson.build` resides:

```
$ meson build --buildtype release
$ cd build
$ ninja
```

If you need debug info and an address sanitizer use this instead:

```
$ meson -Db_sanitize=address build
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

### Example usage

```
$ ./interpreter
yatsukha's lisp
^C to exit, 'help' to get started
detected predef at '.predef.yl', reading...
yl> def {sqr} (\{x} {* x x})
()
yl> sqr 2
4
yl> sqr 2 5
    ^
Excess arguments, expected 1, got 2.
yl> sqr
<fn>: User defined partially evaluated function.
yl> (sqr) 2
4
yl> help \
{
  type: function
  value: <fn>: Lambda function, takes a Q expression argument list, and a Q expression body. For example '(\{x y} {+ x y}) 2 3' will yield 5
}
yl> help def
{
  type: function
  value: <fn>: Global assignment to symbols in a Q expression. For example 'def {a b} 1 2' assigns 1 and 2 to a and b.
}
```

For more information refer to `help`.

## Performance

Aproximately 10^7 operations per second on a single core of a 3.5GHz CPU. Tested using fibonacci.
