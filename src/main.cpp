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

  bool const verbose = argc == 2 && !strcmp(argv[1], "-v");
  
  auto const free_deleter = [](void* ptr) { ::free(ptr); };
  auto const print_error = [](auto&& line, auto&& err) {
    ::std::cout << line << "\n";
    for (::std::size_t i = 0; i < err.column; ++i) {
      ::std::cout << " ";
    }
    ::std::cout << "^" << "\n";
    ::std::cout << err.error_message;
  };

  while (true) {
    ::std::unique_ptr<char[], decltype(free_deleter)> const input{
      ::readline("yl> "), free_deleter
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
        print_error(input.get(), eexpr.error());
      }
    } else {
      print_error(input.get(), expr.error());
    }

    ::std::cout << "\n";
    ::add_history(input.get());
  }

  ::rl_clear_history();

}
