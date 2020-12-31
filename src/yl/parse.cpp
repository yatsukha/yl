#include "yl/util.hpp"
#include <iostream>
#include <yl/either.hpp>
#include <bits/c++config.h>
#include <cctype>
#include <memory>
#include <yl/parse.hpp>

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
    auto const static is_paren = [](auto&& c) {
      return c == '(' || c == ')'
              || c == '{' || c == '}';
    };

    pos const start = curr;
    while (!is_eof(line, curr) && !::std::isblank(line[curr]) 
            && !is_paren(line[curr])) {
      ++curr;
    }

    return succeed(poly_base{new terminal{start, ::std::string(line + start, line + curr)}});
  }

  parse_result parse_expression(char const* line, prf curr,
                                char const close_parenthesis = '\0') noexcept {
    skip(line, curr);
    if (is_eof(line, curr)) {
      return fail(error_info{"Expression expected.", curr});
    }

    auto expr = ::std::make_shared<expression>(curr);
    auto const static left_paren = [](auto&& c) {
      return c == '(' || c == '{';
    };
    auto const static right_paren = [](auto&& c) {
      return c == ')' || c == '}';
    };

    while (!is_eof(line, curr) && !right_paren(line[curr])) {
      if (left_paren(line[curr])) {
        bool const q = line[curr] == '{';
        auto closing = q ? '}' : ')';

        if (auto res = parse_expression(line, ++curr, closing); res) {
          expr->args.emplace_back(res.value());
          dynamic_cast<expression*>(expr->args.back().get())->q = q;
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
      if (!is_eof(line, curr) && right_paren(line[curr])) {
        if (line[curr] != close_parenthesis) {
          return fail(error_info{
            concat("Differing parentesis, expected ", close_parenthesis, " got ",
                   line[curr], "."),
            curr
          });
        }
        ++curr;
      } else {
        return fail(error_info{"Expected closing parenthesis.", curr});
      }
    }
    
    return succeed(poly_base{expr});
  }

  either<error_info, poly_base> parse(char const* line) noexcept {
    pos p = 0;
    auto ret = parse_expression(line, p, false);
    if (!ret) {
      return ret;
    }
    if (line[p] == ')' || line[p] == '}') {
      return fail(error_info{"Unmatched parenthesis.", p});
    }
    return ret;
  }

}
