# Yatsukha's Lisp

 Lisp like interpreted language realized using C++. Being intrigued by the [Build Your Own Lisp](http://www.buildyourownlisp.com) guide, I started writting my own Lisp implementation. Although the guide is quite good for writting in pure C, I was usually just looking at the examples and thinking of how I can figure it out on my own, after all that's where all the fun lies.

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

You can use the `-v` if you want to check that the tokens are parsed properly, although error reporting will messed up for one line.
