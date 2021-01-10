#include "yl/types.hpp"
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

  char left_paren(char const c) noexcept {
    return (c == '(' || c == '{') ? c : 0;
  }

  char right_paren(char const c) noexcept {
    return (c == ')' || c == '}') ? c : 0;
  }

  char paren(char const c) noexcept {
    return left_paren(c) ?: right_paren(c);
  }

  int paren_balance(char const* line) noexcept {
    pos p = 0;
    int balance = 0;

    while (line[p]) {
      if (left_paren(line[p])) {
        --balance;
      } else if (right_paren(line[p])) {
        ++balance;
      }

      ++p;
    }

    return balance;
  }

  result_type parse_raw_string(
      char const* line, ::std::size_t const line_num, prf curr) noexcept {
    pos start = curr;
    string s{.str = {}, .raw = true};

    while (line[curr] != '\"') {
      if (is_eof(line, curr)) {
        return fail(error_info{"Unexpected EOF.", {line_num, curr}});
      }

      if (line[curr] == '\\') {
        ++curr;
        if (is_eof(line, curr)) {
          return fail(error_info{"Unexpected EOF.", {line_num, curr}});
        }
      }

      s.str += line[curr++];
    }

    ++curr;
    return succeed(unit{{line_num, start}, s});
  }

  result_type parse_terminal(
      char const* line, ::std::size_t const line_num, prf curr) noexcept {
    if (line[curr] == '\"') {
      return parse_raw_string(line, line_num, ++curr);
    }

    pos const start = curr;
    while (!is_eof(line, curr) && !::std::isblank(line[curr]) 
            && !paren(line[curr])) {
      ++curr;
    }

    string s{{line + start, line + curr}};

    try {
      return succeed(unit{{line_num, start}, ::std::stol(s.str)});
    } catch (::std::invalid_argument const&) {}

    return succeed(
      unit{{line_num, start}, s}
    );
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

    while (!is_eof(line, curr) && !right_paren(line[curr])) {
      if (!left_paren(line[curr])) {
        if (auto res = parse_terminal(line, line_num, curr); res) {
          ls.children.emplace_back(res.value());
        } else {
          return res;
        }
      } else {
        bool const q = line[curr] == '{';
        auto closing = q ? '}' : ')';

        if (auto res = parse_expression(line, line_num, ++curr, closing); res) {
          auto value = res.value();
          ::std::get<list>(value.expr).q = q;
          ls.children.push_back(value);
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
            {line_num, curr}
          });
        }
        ++curr;
      } else {
        return fail(error_info{"Expected closing parenthesis.", {line_num, curr}});
      }
    }

    expr.expr = ::std::move(ls); 
    return succeed(expr);
  }

  result_type parse(char const* line, ::std::size_t const line_num) noexcept {
    pos p = 0;
    auto ret = parse_expression(line, line_num, p, false);
    if (!ret) {
      return ret;
    }
    if (right_paren(line[p])) {
      return fail(error_info{"Unmatched parenthesis.", {line_num, p}});
    }
    return ret;
  }



}
