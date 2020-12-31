#pragma once

#include <yl/parse.hpp>
#include <yl/either.hpp>

namespace yl {

  using result_type = either<poly_base, double>;
  using eval_either = either<error_info, result_type>;

  eval_either eval(poly_base& expr) noexcept;

}
