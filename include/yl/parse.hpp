#pragma once

#include <ostream>
#include <string>
#include <vector>
#include <variant>

#include <yl/either.hpp>

namespace yl {

  // base types

  struct ls;
  using numeric = double;
  using symbol  = ::std::string;

  using expression = ::std::variant<numeric, symbol, ls>;

  struct parse_unit;

  struct ls {
    bool q = false;
    ::std::vector<parse_unit> children;
  };

  struct parse_unit {
    ::std::size_t pos;
    expression expr;
  };

  struct error_info {
    ::std::string const error_message;
    ::std::size_t const column;
  };

  using parse_result = either<error_info, parse_unit>;

  parse_result parse(char const* line) noexcept;

  ::std::ostream& operator<<(::std::ostream& out, expression const&) noexcept;

}
