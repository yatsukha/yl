#include "yl/util.hpp"
#include <iostream>
#include <stdexcept>
#include <variant>
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

    symbol s(line + start, line + curr);

    try {
      return succeed(parse_unit{start, ::std::stod(s)});
    } catch (::std::invalid_argument const&) {}

    return succeed(parse_unit{start, s});
  }

  parse_result parse_expression(char const* line, prf curr,
                                char const close_parenthesis = '\0') noexcept {
    skip(line, curr);
    if (is_eof(line, curr)) {
      return fail(error_info{"Expression expected.", curr});
    }

    auto list = ls{};
    auto expr = parse_unit{curr};

    auto const static left_paren = [](auto&& c) {
      return c == '(' || c == '{';
    };
    auto const static right_paren = [](auto&& c) {
      return c == ')' || c == '}';
    };

    while (!is_eof(line, curr) && !right_paren(line[curr])) {
      if (!left_paren(line[curr])) {
        auto const old = curr;

        if (auto res = parse_terminal(line, curr); res) {
          list.children.emplace_back(res.value());
          skip(line, curr);
          continue;
        }

        curr = old;
      }

      bool const q = line[curr] == '{';
      auto closing = q ? '}' : ')';

      if (auto res = parse_expression(line, ++curr, closing); res) {
        auto value = res.value();
        ::std::get<ls>(value.expr).q = q;
        list.children.push_back(value);
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
            curr
          });
        }
        ++curr;
      } else {
        return fail(error_info{"Expected closing parenthesis.", curr});
      }
    }

    expr.expr = list;
    
    return succeed(expr);
  }

  parse_result parse(char const* line) noexcept {
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


  ::std::ostream& operator<<(::std::ostream& out, expression const& e) noexcept {
    ::std::visit(overloaded {
      [&out](numeric any) { out << any; },
      [&out](symbol any) { out << any; },
      [&out](ls list) {
        out << (list.q ? "{" : "(");
        for (::std::size_t i = 0; i < list.children.size(); ++i) {
          if (i) {
            out << " ";
          }
          out << list.children[i].expr;
        }
        out << (list.q ? "}" : ")");
      }
    }, e);

    return out;
  }

}
