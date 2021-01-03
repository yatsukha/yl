#pragma once

#include <ostream>
#include <string>
#include <vector>
#include <variant>

#include <yl/types.hpp>
#include <yl/either.hpp>

namespace yl {

  result_type parse(char const* line, ::std::size_t const line_num) noexcept;

}
