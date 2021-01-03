#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <variant>
#include <memory>

#include <yl/either.hpp>

namespace yl {

  // base types

  struct list;
  struct function;

  using numeric = double;
  using symbol  = ::std::string;

  using expression = ::std::variant<numeric, symbol, list, function>;

  ::std::ostream& operator<<(::std::ostream& out, expression const&) noexcept;

  using environment = ::std::unordered_map<symbol, expression>;
  using env_ptr     = ::std::shared_ptr<environment>;

  struct env_node;

  using env_node_ptr = ::std::shared_ptr<env_node>;

  struct env_node {
    env_ptr curr;
    env_node_ptr prev;
  };

  env_node_ptr global_environment() noexcept;

  // helpers
  
  struct position {
    ::std::size_t line;
    ::std::size_t column;
  };
  
  struct error_info {
    ::std::string const error_message;
    position const pos;
  };

  struct unit;

  using result_type = either<error_info, unit>;

  //

  struct list {
    bool q = false;
    ::std::vector<unit> children;
  };

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
