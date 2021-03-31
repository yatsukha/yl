#pragma once

#include <yl/types.hpp>

namespace yl {

  result_type parse(char const* line, ::std::size_t const line_num) noexcept;

  int paren_balance(char const* line) noexcept;

  ::std::size_t skip_str(char const* line, ::std::size_t pos) noexcept;

}
