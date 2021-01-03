#include <cstdlib>
#include <cstring>
#include <fstream>
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

namespace yl {

  void print_error(
    ::std::size_t const prompt_offset,
    error_info const& err,
    ::std::ostream& std_err
  ) noexcept {
    auto const past = ::history_length - err.pos.line;
    
    if (past) {
      auto** list = ::history_list();
      ::std::cout << past << " entries ago:" << "\n";
      ::std::cout << list[err.pos.line]->line << "\n";
    }

    for (::std::size_t i = 0; 
         i != err.pos.column + (past ? 0 : prompt_offset); ++i) {
      std_err << " ";
    }
    std_err << "^" << "\n";
    std_err << err.error_message;
  }

  void handle_input(
    char const* user_input,
    ::std::size_t const prompt_offset,
    ::std::ostream& std_out,
    ::std::ostream& std_err
  ) noexcept {
    auto const parse_expr = parse(user_input, ::history_length);
    
    if (!parse_expr) {
      print_error(prompt_offset, parse_expr.error(), std_err);
      return;
    }

    auto const eval_expr = eval(parse_expr.value());

    if (!eval_expr) {
      print_error(prompt_offset, eval_expr.error(), std_err);
      return;
    }

    std_out << eval_expr.value().expr; 
  }

}

int main(int const argc, char const* const* argv) {
  ::std::cout << "yatsukha's lisp" << "\n";
  ::std::cout << "^C to exit" << "\n";

  ::std::string predef_name = ".predef.yl";

  ::std::ifstream predef(predef_name);

  if (predef.is_open()) {
    ::std::cout << "detected predef at '" << predef_name
                << "', reading..."
                << "\n";
    ::std::size_t buf_size = 1024;
    char* arr = new char[buf_size];
    ::std::ostream dev_null{nullptr};
    while (predef.getline(arr, buf_size)) {
      if (arr[0]) {
        ::yl::handle_input(arr, 0, dev_null, ::std::cerr);
        ::add_history(arr);
      }
    }
  }

  ::std::string prompt = "yl> ";

  while (true) {
    auto const* input = ::readline(prompt.c_str());

    if (!input[0]) {
      continue;
    }

    ::yl::handle_input(input, prompt.size(), ::std::cout, ::std::cerr);

    ::std::cout << "\n";
    ::add_history(input);
  }

  ::rl_clear_history();
}
