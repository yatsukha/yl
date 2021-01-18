#pragma once

#include "yl/types.hpp"
#include "yl/util.hpp"
#include <string>
#include <functional>
#include <utility>
#include <variant>

#include <yl/parse.hpp>
#include <yl/either.hpp>

namespace yl {

  result_type resolve_symbol(unit_ptr const& pu, env_node_ptr node) noexcept;

  result_type eval(
    unit_ptr const& pu, 
    env_node_ptr node = global_environment(), 
    bool const force_eval = false
  ) noexcept;
  
}
