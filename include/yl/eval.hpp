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

  result_type resolve_symbol(unit const& pu, env_node_ptr node) noexcept;

  result_type eval(
    unit const& pu, 
    env_node_ptr node = global_environment(), 
    bool const eval_q = false
  ) noexcept;
  
}
