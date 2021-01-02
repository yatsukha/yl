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

  using resolved_symbol       = expression;
  using resolve_symbol_result = result_type;

  // TODO: add functional state using a struct
  resolve_symbol_result resolve_symbol(unit const& pu) noexcept;

  result_type eval(unit const& pu, bool const eval_q = false) noexcept;
  
}
