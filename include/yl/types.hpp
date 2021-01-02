#pragma once

#include <string>
#include <functional>
#include <variant>

#include <yl/either.hpp>

namespace yl {

  // base types

  struct ls;
  struct function;

  using numeric = double;
  using symbol  = ::std::string;

  using expression = ::std::variant<numeric, symbol, ls, function>;

  // helpers
  
  struct error_info {
    ::std::string const error_message;
    ::std::size_t const column;
  };

  struct unit;

  using result_type = either<error_info, unit>;

  //

  struct ls {
    bool q = false;
    ::std::vector<unit> children;
  };



  struct function {
    ::std::string description;
    ::std::function<result_type(unit)> func;
  };

  struct unit {
    ::std::size_t pos;
    expression expr;
  };

  ::std::ostream& operator<<(::std::ostream& out, expression const&) noexcept;

}
