#include <sstream>
#include <yl/user_io.hpp>
#include <yl/parse.hpp>
#include <yl/eval.hpp>
#include <yl/util.hpp>
#include <yl/types.hpp>
#include <yl/mem.hpp>
#include <yl/history.hpp>

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

namespace yl {

  void print_error(
    ::std::size_t const prompt_offset,
    bool const continuated,
    error_info const& err,
    ::std::ostream& std_err
  ) noexcept {
    auto const past =
      history::size() - err.pos.line - 1;
    
    if (past || continuated) {
      if (past) {
        std_err << past << " entries ago:" << "\n";
      }
      std_err << history::get(err.pos.line) << "\n";
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
    auto const parse_expr = parse(user_input, history::size());
    history::append(user_input);

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
        } else {
          idx = skip_str(line, idx);
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

  void handle_file(
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

  string_representation load_predef(      
    string_representation const& predef
    ) noexcept {
    ::std::ifstream in{predef.c_str()};
    if (!in.is_open()) {
      return make_string("could not open predef file");
    }
    ::std::stringstream ss;
    ::std::ostream dev_null{nullptr};
    handle_file(::std::move(in), dev_null, ss);

    return make_string(
      ss.str().size()
        ? "errors in predef, please interpret it directly for more details"
        : ""
    );
  }

}
