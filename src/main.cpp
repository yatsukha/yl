#include <cstdlib>
#include <cstring>
#include <ios>
#include <iostream>
#include <limits>
#include <string>
#include <memory>

#include <readline/readline.h>
#include <readline/history.h>

#include <yl/parser.hpp>
#include <yl/eval_polish.hpp>

int main(int const argc, char const* const* argv) {

  bool const print_tree = argc == 2 && !strcmp(argv[1], "-v");

  ::std::cout << "yatsukha's lisp" << "\n";
  
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

    if (auto expr = ::yl::parse_polish(input.get()); expr) {
      if (print_tree) {
        expr.value()->print(::std::cout); ::std::cout << "\n";
      }

      if (auto res = ::yl::eval(expr.value()); res) {
        ::std::cout << "Result: " << res.value();
      } else {
        print_error(input.get(), res.error());
      }
    } else {
      print_error(input.get(), expr.error());
    }

    ::std::cout << "\n";
    ::add_history(input.get());
  }

  ::rl_clear_history();

}
