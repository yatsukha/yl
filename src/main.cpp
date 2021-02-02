#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <memory>
#include <utility>
#include <variant>

#include <readline/readline.h>
#include <readline/history.h>

#include <yl/parse.hpp>
#include <yl/either.hpp>
#include <yl/eval.hpp>
#include <yl/util.hpp>
#include <yl/types.hpp>
#include <yl/mem.hpp>

namespace yl {

  void print_error(
    ::std::size_t const prompt_offset,
    bool const continuated,
    error_info const& err,
    ::std::ostream& std_err
  ) noexcept {
    auto const past = ::history_length - err.pos.line - 1;
    
    if (past || continuated) {
      auto** list = ::history_list();
      if (past) {
        std_err << past << " entries ago:" << "\n";
      }
      std_err << list[err.pos.line]->line << "\n";
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
    bool const continuated,
    ::std::ostream& std_out,
    ::std::ostream& std_err
  ) noexcept {
    auto const parse_expr = parse(user_input, ::history_length);
    ::add_history(user_input);
    
    if (!parse_expr) {
      print_error(prompt_offset, continuated, parse_expr.error(), std_err);
      return;
    }

    auto const eval_expr = eval(parse_expr.value());

    if (!eval_expr) {
      print_error(prompt_offset, continuated, eval_expr.error(), std_err);
      return;
    }

    std_out << eval_expr.value()->expr; 
  }

  namespace detail {

    // true if empty
    bool preproccess(char* line) {
      bool empty = true;
      ::std::size_t idx = 0;
      while (line[idx]) {
        if (line[idx] == ';') {
          line[idx] = 0;
          return empty;
        }

        empty = empty && ::std::isblank(line[idx]);
        ++idx;
      }

      return empty;
    }

  }

  // true if eof
  bool handle_line(
    ::std::function<char*()> line_supplier, 
    ::std::size_t const p_sz,
    ::std::ostream& out,
    ::std::ostream& err
  ) noexcept {
    ::std::size_t constexpr static buf_size = 1 << 13;
    char buf[buf_size];
    buf[0] = 0;

    auto* input = line_supplier();

    if (!input) {
      return true;
    }

    if (::yl::detail::preproccess(input)) {
      ::free(input);
      return false;
    }

    auto balance = ::yl::paren_balance(input);
    auto continuated = balance < 0;

    ::std::strcat(buf, input);
    ::free(input);

    while (balance < 0) {
      input = line_supplier();
      if (!input) {
        break;
      }

      if (::yl::detail::preproccess(input)) {
        ::free(input);
        continue; 
      }
      balance += ::yl::paren_balance(input);
      ::std::strcat(buf, " ");
      ::std::strcat(buf, input);
      ::free(input);
    }

    if (!p_sz) {
      out << buf << "\n";
    }

    ::yl::handle_input(buf, p_sz, continuated, out, err);
    out << "\n";

    if (!p_sz) {
      out << "\n";
    }

    return false;
  }

  int insert_spaces(int, int) {
    ::rl_insert_text("  ");
    return 0;
  }

  auto handle_file(
    ::std::ifstream&& file,
    ::std::ostream& out,
    ::std::ostream& err
  ) noexcept {
    while (!::yl::handle_line(
      [&file] { 
        ::std::size_t constexpr static buf_size = 1 << 13;
        char* buf = reinterpret_cast<char*>(::malloc(buf_size));

        if (file.getline(buf, buf_size)) {
          return buf;
        }
        
        ::free(buf);
        return static_cast<char*>(nullptr);
      }, 
      0, out, err
    ))
      ;
  }

}

int main(int const argc, char const* const* argv) {
  ::rl_bind_key('\t', ::yl::insert_spaces);

  ::std::cout << "yatsukha's lisp" << "\n";
  ::std::cout << "^C to exit, 'help' to get started" << "\n";

  ::std::string predef_name = ".predef.yl";
  ::std::ifstream predef(predef_name);

  if (predef.is_open()) {
    ::std::ostream dev_null{nullptr};

    ::std::cout << "detected predef at '" << predef_name
                << "', reading..."
                << "\n";

    ::yl::handle_file(::std::move(predef), dev_null, ::std::cerr);
  }

  ::std::ifstream input_file;

  if (argc > 1) {
    input_file = ::std::ifstream{argv[1]};
    if (input_file.is_open()) {
      ::std::cout << "interpreting '" << argv[1] << "'" << "\n\n";
      ::yl::handle_file(::std::move(input_file), ::std::cout, ::std::cerr);
    } else {
      ::std::cerr << "unable to open given file for interpretation: "
                  << argv[1]
                  << "\n"
                  << "exiting"
                  << "\n"; 
    }
  } else {
    ::std::string prompt = "yl> ";
    ::std::string continuation_prompt = "... ";

    while (!::yl::handle_line(
      [i = 0, &prompt, &continuation_prompt]() mutable {
        return ::readline((i++ ? continuation_prompt : prompt).c_str());
      }, 
      prompt.size(), 
      ::std::cout,
      ::std::cerr
    ))
      ;
  }

  ::rl_clear_history();
}
