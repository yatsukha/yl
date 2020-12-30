#pragma once

#include <cstdlib>
#include <sstream>
#include <string>
#include <utility>
#include <iostream>

namespace yl {
  
  template<typename... Args>
  inline void terminate_with(Args&& ...args) noexcept {
    (..., (::std::cerr << args << " ")) << ::std::endl; // flush
    ::std::exit(EXIT_FAILURE);
  }

  namespace detail {

    template<typename... Args>
    void cat(::std::stringstream& ss, Args&& ...args) noexcept {
      (..., (ss << args));
    }

  }

  template<typename... Args>
  ::std::string concat(Args&& ...args) noexcept {
    ::std::stringstream ss;
    detail::cat(ss, ::std::forward<Args>(args)...);
    return ss.str();
  }

}
