#include <yl/either.hpp>
#include <bits/c++config.h>
#include <cctype>
#include <memory>
#include <yl/parser.hpp>

namespace yl {

  using pos = ::std::size_t;
  using prf = pos&;

  bool is_eof(char const* line, pos const curr) noexcept {
    return !line[curr];
  }

  // skips whitespace
  void skip(char const* line, prf curr) noexcept {
    if (is_eof(line, curr)) {
      return;
    }

    while (::std::isblank(line[curr])) {
      ++curr;
    }
  }

  parse_result parse_terminal(char const* line, prf curr) noexcept { 
    pos const start = curr;
    while (!is_eof(line, curr) && !::std::isblank(line[curr]) 
            && line[curr] != '(' && line[curr] != ')') {
      ++curr;
    }

    if (curr == start) {
      return fail(error_info{"Expected a token.", curr});
    }

    return succeed(poly_base{new terminal{start, ::std::string(line + start, line + curr)}});
  }

  parse_result parse_expression(char const* line, prf curr,
                                bool const close_parenthesis = true) noexcept {
    skip(line, curr);
    if (is_eof(line, curr)) {
      return fail(error_info{"Expression expected.", curr});
    }

    auto expr = ::std::make_shared<expression>(curr);

    while (!is_eof(line, curr) && line[curr] != ')') {
      if (line[curr] == '(') {
        if (auto res = parse_expression(line, ++curr); !res.is_error) {
          expr->args.emplace_back(res.value());
        } else {
          return res;
        }
      } else {
        if (auto res = parse_terminal(line, curr); !res.is_error) {
          expr->args.emplace_back(res.value());
        } else {
          return res;
        }
      }
      skip(line, curr);
    }

    if (close_parenthesis) {
      if (!is_eof(line, curr) && line[curr] == ')') {
        ++curr;
      } else {
        return fail(error_info{"Expected closing parenthesis.", curr});
      }
    }

    if (expr->args.empty()) {
      return fail(error_info{"Expression expected.", curr});
    }
    
    return succeed(poly_base{expr});
  }

  either<error_info, poly_base> parse_polish(char const* line) noexcept {
    pos p = 0;
    bool close_parenthesis = false;
    if (line[p] == '(') {
      ++p;
      close_parenthesis = true;
    }
    return parse_expression(line, p, close_parenthesis);
  }

}
