#pragma once

#include <yl/types.hpp>

#include <ostream>

namespace yl {

  void print_error(
    ::std::size_t const prompt_offset,
    bool const continuated,
    error_info const& err,
    ::std::ostream& std_err
  ) noexcept; 

  void handle_input(
    char const* user_input,
    ::std::size_t const prompt_offset,
    bool const continuated,
    ::std::ostream& std_out,
    ::std::ostream& std_err
  ) noexcept; 


  // true if eof
  bool handle_line(
    ::std::function<char*()> line_supplier, 
    ::std::size_t const p_sz,
    ::std::ostream& out,
    ::std::ostream& err
  ) noexcept; 

}
