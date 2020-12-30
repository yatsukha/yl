#pragma once

#include <yl/parser.hpp>
#include <yl/either.hpp>

namespace yl {

  either<error_info, double> eval(poly_base const& expr) noexcept;

}
