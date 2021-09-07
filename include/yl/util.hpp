#pragma once

#include <cstddef>
#include <cstdlib>
#include <sstream>
#include <string>
#include <utility>
#include <iostream>
#include <variant>

#include <yl/mem.hpp>

namespace yl {
  
  template<typename... Args>
  [[noreturn]] inline void terminate_with(Args&& ...args) noexcept {
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
  string_representation concat(Args&& ...args) noexcept {
    ::std::stringstream ss; // C++20 when
    detail::cat(ss, ::std::forward<Args>(args)...);
    return make_string(ss.str().c_str());
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

  template<typename F>
  auto make_scope_guard(F&& f) noexcept {
    auto deleter = [self = ::std::forward<F>(f)](auto&&) {
      self();
    };
    return ::std::unique_ptr<void, decltype(deleter)>{
      reinterpret_cast<void*>(-1),
      ::std::move(deleter)
    };
  }

}
