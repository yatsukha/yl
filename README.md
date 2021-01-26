# My own Lisp-like language

 Lisp like interpreted language realized using C++. Being intrigued by the [Build Your Own Lisp](http://www.buildyourownlisp.com) guide, I started writting my own implementation. Although the guide is quite good for writting in pure C, I was usually just looking at the examples and thinking of how I can figure it out on my own, after all that's where all the fun lies.

## Features

This section just lists interesting features, not all of them.

 * code as value
 * predef
 * basic error reporting, it will also print the line where the error is if it was entered previously in the interpreter
 * variadic arguments for user defined functions
 * partially evaluated user defined functions: if the function is not variadic and you pass less than the required number of arguments the function will be bound to those arguments
 * recursion

## Building

Requires:
  * ninja
  * meson
  * C++17 compiler
  * readline

It is recommended to build for release if you plan on running more complex programs.
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
yl> fun {** base exponent} {unpack * (repeat exponent base)}
()
yl> ** 2 4
16
yl> def {binary-pow} (** 2)
()
yl> binary-pow 4
16
yl> help def
"
function:
Defines a global variable. 'def {a b} 1 2' assigns 1 and 2 to a and b.
"
yl> help binary-pow
"
function:
User defined partially evaluated function.
"
yl>
```

For more information refer to `help`.

## Performance

Optimization of the interpreter went as follows (assisted by perf):
  1. Everything was written using value based semantics, without pointers, every function took a copy of an expression.
  2. Functions were rewritten to use references instead of copy. This yielded very small performance gains. Obviously the compiler was very good at optimizing value based arguments passing.
  3. The main bottleneck after 2. was returning copies of expressions, which mostly revolved around copying lists with __heavy__ children. I rewrote everything so that a single pool resource is used with shared pointers, this resulted in at least a 4x speedup.
  4. The program still spent a large amount of its runtime indexing into an unordered hash table with a string. I somewhat mitigated this by having a really fast and dumb hash function that just xors the first two chars in the string.
  5. Very deep recursive calls resulted in having to look through a large number of environments to find a symbol that was defined in the global environment. To fix this I disregarded environments that don't differ in symbols, for example if a function has symbols 'a' and 'b' in its environment, it does need the callers environment that also has only 'a' and 'b' in its environment. This is most apparent with recursive functions. Disregarding the parent environment in these cases resulted in extreme improvements (5x speedup in some cases) for some functions. The best example of this was the 'split' function in predef which does everything recursively.

## Future work

To achieve better performance:
  * tail recursion optimization
  * more builtins that do not rely on recursion

Language features:
  * proper closures
  * more numeric types
