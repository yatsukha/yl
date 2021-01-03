#include <iostream>
#include <stdexcept>
#include <variant>
#include <cctype>
#include <memory>

#include <yl/either.hpp>
#include <yl/util.hpp>
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

  result_type parse_terminal(
      char const* line, ::std::size_t const line_num, prf curr) noexcept { 
    auto const static is_paren = [](auto&& c) {
      return c == '(' || c == ')'
              || c == '{' || c == '}';
    };

    pos const start = curr;
    while (!is_eof(line, curr) && !::std::isblank(line[curr]) 
            && !is_paren(line[curr])) {
      ++curr;
    }

    symbol s(line + start, line + curr);

    try {
      return succeed(unit{{line_num, start}, ::std::stod(s)});
    } catch (::std::invalid_argument const&) {}

    return succeed(unit{{line_num, start}, s});
  }

  result_type parse_expression(
      char const* line, ::std::size_t const line_num, prf curr,
      char const close_parenthesis = '\0') noexcept {
    skip(line, curr);
    if (is_eof(line, curr)) {
      return fail(error_info{"Expression expected.", {line_num, curr}});
    }

    auto ls = list{};
    auto expr = unit{{line_num, curr}};

    auto const static left_paren = [](auto&& c) {
      return c == '(' || c == '{';
    };
    auto const static right_paren = [](auto&& c) {
      return c == ')' || c == '}';
    };

    while (!is_eof(line, curr) && !right_paren(line[curr])) {
      if (!left_paren(line[curr])) {
        auto const old = curr;

        if (auto res = parse_terminal(line, line_num, curr); res) {
          ls.children.emplace_back(res.value());
          skip(line, curr);
          continue;
        }

        curr = old;
      }

      bool const q = line[curr] == '{';
      auto closing = q ? '}' : ')';

      if (auto res = parse_expression(line, line_num, ++curr, closing); res) {
        auto value = res.value();
        ::std::get<list>(value.expr).q = q;
        ls.children.push_back(value);
      } else {
        return res;
      }

      skip(line, curr);
    }

    if (close_parenthesis) {
      if (!is_eof(line, curr) && right_paren(line[curr])) {
        if (line[curr] != close_parenthesis) {
          return fail(error_info{
            concat("Differing parentesis, expected ", close_parenthesis, " got ",
                   line[curr], "."),
            {line_num, curr}
          });
        }
        ++curr;
      } else {
        return fail(error_info{"Expected closing parenthesis.", {line_num, curr}});
      }
    }

    expr.expr = ls;
    
    return succeed(expr);
  }

  result_type parse(char const* line, ::std::size_t const line_num) noexcept {
    pos p = 0;
    auto ret = parse_expression(line, line_num, p, false);
    if (!ret) {
      return ret;
    }
    if (line[p] == ')' || line[p] == '}') {
      return fail(error_info{"Unmatched parenthesis.", {line_num, p}});
    }
    return ret;
  }



}
