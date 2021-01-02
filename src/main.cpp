#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <memory>
#include <variant>

#include <readline/readline.h>
#include <readline/history.h>

#include <yl/parse.hpp>
#include <yl/either.hpp>
#include <yl/eval.hpp>
#include <yl/util.hpp>

int main(int const argc, char const* const* argv) {
  ::std::cout << "yatsukha's lisp" << "\n";
  ::std::cout << "^C to exit" << "\n";

  char const* prompt = "yl> ";

  bool const verbose = argc == 2 && !strcmp(argv[1], "-v");
  
  auto const free_deleter = [](void* ptr) { ::free(ptr); };
  auto const print_error = 
  [offset = ::std::strlen(prompt)](auto&& err) {
    for (::std::size_t i = 0; i < err.column + offset; ++i) {
      ::std::cout << " ";
    }
    ::std::cout << "^" << "\n";
    ::std::cout << err.error_message;
  };

  while (true) {
    ::std::unique_ptr<char[], decltype(free_deleter)> const input{
      ::readline(prompt), free_deleter
    };

    if (!input[0]) {
      continue;
    }

    if (auto expr = ::yl::parse(input.get()); expr) {
      if (verbose) {
        ::std::cout << expr.value().expr << "\n";
      }

      if (auto eexpr = ::yl::eval(expr.value()); eexpr) {
        ::std::cout << eexpr.value().expr;
      } else {
        print_error(eexpr.error());
      }
    } else {
      print_error(expr.error());
    }

    ::std::cout << "\n";
    ::add_history(input.get());
  }

  ::rl_clear_history();
}
