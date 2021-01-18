#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>
#include <utility>
#include <variant>
#include <memory>

#include <yl/either.hpp>
#include <yl/mem.hpp>

namespace yl {

  // base types
  
  using string_representation = ::std::pmr::string;

  struct string {
    string_representation str{&mem_pool};
    bool raw = false;
  };

  struct list;
  struct function;

  using numeric = ::std::int64_t;
  using expression = ::std::variant<numeric, string, list, function>;

  ::std::ostream& operator<<(::std::ostream& out, expression const&) noexcept;
  ::std::string type_of(expression const&) noexcept;

  // environment

  struct unit;
  using unit_ptr = ::std::shared_ptr<unit>;

  struct str_hasher {
    ::std::size_t operator()(string_representation const& str) const noexcept {
      switch (str.size()) {
        case 0:
          return 0;
        case 1:
          return str[0];
        default:
          return str[0] ^ str[1];
      }
    }
  };

  using environment = 
    ::std::pmr::unordered_map<string_representation, unit_ptr, str_hasher>;
  using env_ptr = ::std::shared_ptr<environment>;

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
    using children_type = ::std::pmr::vector<unit_ptr>;
    children_type children{&mem_pool};
  };

  using result_type = either<error_info, unit_ptr>;

  struct function {
    using type = ::std::function<result_type(unit_ptr const&, env_node_ptr&)>;
    string_representation description;
    type func;
  };

  struct unit {
    position pos;
    expression expr;
  };

#define DEF_CAST(type) \
  inline type& as_##type(expression& expr) noexcept { \
    return ::std::get<type>(expr); \
  } \
  inline type const& as_##type(expression const& expr) noexcept { \
    return ::std::get<type>(expr); \
  }

  DEF_CAST(numeric);
  DEF_CAST(string);
  DEF_CAST(function);
  DEF_CAST(list);

#define DEF_TYPE_CHECK(type) \
  inline bool is_##type(expression const& expr) noexcept { \
    return ::std::holds_alternative<type>(expr); \
  }

  DEF_TYPE_CHECK(numeric);
  DEF_TYPE_CHECK(string);
  DEF_TYPE_CHECK(function);
  DEF_TYPE_CHECK(list);

}
