#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>
#include <utility>
#include <variant>
#include <memory>

#include <yl/either.hpp>

namespace yl {

  // base types

  struct list;
  struct function;

  using numeric = ::std::int64_t;
  using symbol  = ::std::string;

  using expression = ::std::variant<numeric, symbol, list, function>;

  ::std::ostream& operator<<(::std::ostream& out, expression const&) noexcept;
  ::std::string type_of(expression const&) noexcept;

  // environment

  using environment = ::std::unordered_map<symbol, expression>;
  using env_ptr     = ::std::shared_ptr<environment>;

  struct env_node;

  using env_node_ptr = ::std::shared_ptr<env_node>;

  struct env_node {
    env_ptr curr;
    env_node_ptr prev;
  };

  env_node_ptr global_environment() noexcept;

  // helpers for error reporting
  
  struct position {
    ::std::size_t line;
    ::std::size_t column;
  };
  
  struct error_info {
    ::std::string const error_message;
    position const pos;
  };

  // other type definitions
  
  struct unit;

  struct list {
    bool q = false;
    ::std::vector<unit> children;
  };

  using result_type = either<error_info, unit>;

  struct function {
    using type = ::std::function<result_type(unit, env_node_ptr)>;
    ::std::string description;
    type func;
  };

  struct unit {
    position pos;
    expression expr;
  };

}
