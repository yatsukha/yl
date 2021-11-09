#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <utility>
#include <variant>

#include <immer/map.hpp>
#include <immer/map_transient.hpp>

#include <yl/either.hpp>
#include <yl/mem.hpp>

namespace yl {

  // base types

  // holds a type and metadata
  struct unit;
  using unit_ptr = ::std::shared_ptr<unit>;

  // struct
  struct unit_hasher {
    ::std::size_t operator()(unit_ptr const&) const noexcept;
  };

  struct string {
    string_representation str = make_string();
    bool raw = false;
  };

  struct list;
  struct function;
  using numeric = ::std::int64_t;
  using hash_map = ::immer::map<unit_ptr, unit_ptr, unit_hasher>;

  using expression = ::std::variant<numeric, string, list, function, hash_map>;

  ::std::ostream& operator<<(::std::ostream& out, expression const&) noexcept;
  bool operator==(unit_ptr const&, unit_ptr const&) noexcept;

  ::std::string type_of(expression const&) noexcept;

  // environment

  using environment = 
    PMR_PREF::unordered_map<string_representation, unit_ptr, str_hasher>;

  using env_ptr      = ::std::shared_ptr<environment>;
  using env_node_ptr = ::std::shared_ptr<struct env_node>;

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
    string_representation const error_message;
    position const pos;

    friend ::std::ostream& operator<<(::std::ostream& out, error_info const&) noexcept {
      return out;
    }
  };

  // definitions, other

  struct list {
    bool q = false;
    using children_type = seq_representation<unit_ptr>;
    children_type children = make_seq<unit_ptr>();
  };

  template<typename Result>
  using error_either = either<error_info, Result>;
  using result_type = error_either<unit_ptr>;

  struct function {
    using type = ::std::function<result_type(unit_ptr const&, env_node_ptr&)>;
    string_representation description = make_string();
    type func;

    bool macro = false;
  };

  struct unit {
    position pos;
    expression expr;
  };

}
