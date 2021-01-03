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
#include <yl/types.hpp>

int main(int const argc, char const* const* argv) {
  ::std::cout << "yatsukha's lisp" << "\n";
  ::std::cout << "^C to exit" << "\n";

  char const* prompt = "yl> ";
  ::std::size_t current_line = 0;

  bool const verbose = argc == 2 && !strcmp(argv[1], "-v");
  auto const print_error = 
  [&current_line, offset = ::std::strlen(prompt)](auto&& err) {
    auto const past = current_line - err.pos.line;

    if (past) {
      auto** list = ::history_list();
      ::std::cout << past << " entries ago:" << "\n";
      ::std::cout << list[err.pos.line]->line << "\n";
    }
    for (::std::size_t i = 0; i != err.pos.column + (past ? 0 : offset); ++i) {
      ::std::cout << " ";
    }
    ::std::cout << "^" << "\n";
    ::std::cout << err.error_message;
  };


  while (true) {
    auto const* input = ::readline(prompt);

    if (!input[0]) {
      continue;
    }

    if (auto expr = ::yl::parse(input, current_line); expr) {
      if (auto eexpr = ::yl::eval(expr.value()); eexpr) {
        ::std::cout << eexpr.value().expr;
      } else {
        print_error(eexpr.error());
      }

      if (verbose) {
        ::std::cout << "\n";
        ::std::cout << "Parsed as: " << expr.value().expr;
      }
    } else {
      print_error(expr.error());
    }

    ::std::cout << "\n";
    ::add_history(input);
    ++current_line;
  }

  ::rl_clear_history();
}
