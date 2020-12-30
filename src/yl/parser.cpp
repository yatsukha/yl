#include <iostream>
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
        if (auto res = parse_expression(line, ++curr); res) {
          expr->args.emplace_back(res.value());
        } else {
          return res;
        }
      } else {
        if (auto res = parse_terminal(line, curr); res) {
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
    
    return succeed(expr->copy());
  }

  either<error_info, poly_base> parse_polish(char const* line) noexcept {
    pos p = 0;
    auto ret = parse_expression(line, p, false);
    //return ret;
    if (line[p] == ')') {
      return fail(error_info{"Unmatched parenthesis.", p});
    }
    return ret;
  }

}
