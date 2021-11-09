#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <cctype>

#include <yl/types.hpp>
#include <yl/either.hpp>
#include <yl/util.hpp>
#include <yl/parse.hpp>
#include <yl/mem.hpp>
#include <yl/type_operations.hpp>

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

  ::std::size_t skip_str(char const* line, ::std::size_t pos) noexcept {
    if (!line[pos] || line[pos] != '\"') {
      return pos;
    }
    while (line[++pos] && line[pos] != '\"') {
      if (line[pos] == '\\') {
        ++pos;
      }
    }
    return pos;
  }

  int paren_balance(char const* line) noexcept {
    pos p = 0;
    int balance = 0;

    while (line[p]) {
      if (line[p] == '\"') {
        p = skip_str(line, p);
        if (!line[p]) {
          break;
        }
      }
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
    ::std::unordered_map<char, char> const static escaped{
      {'n', '\n'}, 
      {'t', '\t'}, 
      {'r', '\t'}, 
      {'v', '\v'},
    };


    pos start = curr;
    string s;
    s.raw = true;

    while (line[curr] != '\"') {
      if (is_eof(line, curr)) {
        return fail(error_info{"Unexpected EOF.", {line_num, curr}});
      }

      if (line[curr] == '\\') {
        ++curr;
        if (is_eof(line, curr)) {
          return fail(error_info{"Unexpected EOF.", {line_num, curr}});
        }
        if (auto const iter = escaped.find(line[curr]); iter != escaped.end()) {
          s.str += iter->second;
        } else {
          s.str += line[curr];
        }
      } else {
        s.str += line[curr];
      }

      ++curr;
    }

    ++curr;
    SUCCEED_WITH((position{line_num, start}), s);
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

    // non null char** required for strtoll
    char* eptr = reinterpret_cast<char*>(1);
    char const* sptr = s.str.c_str();

    auto const n = ::std::strtoll(sptr, &eptr, 10);

    if (eptr > sptr && (eptr - sptr > 1 || ::std::isdigit(*(sptr)))) {
      if (eptr != sptr + s.str.size()) {
        FAIL_WITH(
          "Invalid number format. Expected a signed integer.", 
          (position{line_num, start + (eptr - sptr)})
        );
      } else if (errno == ERANGE) {
        errno = 0;
        FAIL_WITH(
          "Given number does not fit into a 64bit signed integer.",
          (position{line_num, start + (eptr - sptr)})
        );
      }

      SUCCEED_WITH((position{line_num, start}), n);
    }

    SUCCEED_WITH((position{line_num, start}), s);
  }

  result_type parse_expression(
      char const* line, ::std::size_t const line_num, prf curr,
      char const close_parenthesis = '\0') noexcept {
    skip(line, curr);
    if (is_eof(line, curr)) {
      FAIL_WITH("Expression expected.", (position{line_num, curr}));
    }

    auto ls = list{};
    auto expr = unit{{line_num, curr}};

    while (!is_eof(line, curr) && !right_paren(line[curr])) {
      if (!left_paren(line[curr])) {
        auto res = parse_terminal(line, line_num, curr);
        RETURN_IF_ERROR(res);
        ls.children.emplace_back(res.value());
      } else {
        bool const q = line[curr] == '{';

        auto res = parse_expression(line, line_num, ++curr, q ? '}' : ')');
        RETURN_IF_ERROR(res);

        as_list(res.value()->expr).q = q;
        ls.children.emplace_back(res.value());
      }

      skip(line, curr);
    }

    if (close_parenthesis) {
      if (!is_eof(line, curr) && right_paren(line[curr])) {
        if (line[curr] != close_parenthesis) {
          FAIL_WITH(
            concat("Differing parentesis, expected ", close_parenthesis, " got ",
                   line[curr], "."),
            (position{line_num, curr}));
        }
        ++curr;
      } else {
        FAIL_WITH("Expected closing parenthesis.", (position{line_num, curr}));
      }
    }

    expr.expr = ::std::move(ls); 
    return succeed(make_shared(::std::move(expr)));
  }

  result_type parse(char const* line, ::std::size_t const line_num) noexcept {
    pos p = 0;
    auto ret = parse_expression(line, line_num, p, false);
    RETURN_IF_ERROR(ret);
    if (right_paren(line[p])) {
      FAIL_WITH("Unmatched parenthesis.", (position{line_num, p}));
    }
    return ret;
  }

}
