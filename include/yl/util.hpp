#pragma once

#include <cstdlib>
#include <sstream>
#include <string>
#include <utility>
#include <iostream>
#include <variant>

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

  template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
  template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

  template<typename V, typename T>
  struct append;

  template<typename... Args, typename T>
  struct append<::std::variant<Args...>, T> {
    using type = ::std::variant<Args..., T>;
  };

  template<typename V, typename T>
  using append_t = typename append<V, T>::type;

}
