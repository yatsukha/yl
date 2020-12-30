#pragma once

#include <yl/util.hpp>
#include <type_traits>
#include <utility>

namespace yl {

  template<typename E, typename S = void>
//           typename = ::std::enable_if_t<
//             ::std::is_trivially_destructible_v<E> || ::std::is_same_v<void, E>>,
//           typename = ::std::enable_if_t<
//             ::std::is_trivially_destructible_v<S> || ::std::is_same_v<void, S>>>
  class either {
    using _E = ::std::conditional_t<::std::is_same_v<void, E>, char, E>;  
    auto constexpr static sz_e = sizeof(_E);
    using _S = ::std::conditional_t<::std::is_same_v<void, S>, char, S>;
    auto constexpr static sz_s = sizeof(_S);
    auto constexpr static max_sz = ::std::max(sz_e, sz_s);

   public:
    // the price to pay
    alignas(::std::max(alignof(_E), alignof(_S))) char data[max_sz];
    bool is_error;

    explicit either(bool const is_error = true) noexcept 
      : is_error(is_error) {}

    ~either() {
      if (is_error) {
        if constexpr (!::std::is_same_v<void, E>) {
          (*reinterpret_cast<E*>(data)).~E();
        }
      } else {
        if constexpr (!::std::is_same_v<void, S>) {
          (*reinterpret_cast<S*>(data)).~S();
        }
      }
    }

    template<typename EE, typename = ::std::enable_if<::std::is_same_v<void, E>>>
    operator either<EE, S>() const noexcept {
      if (is_error) {
        terminate_with("Invalid widening from a void error value.");
      }  
  
      either<EE, S> ret{is_error};
      if constexpr (!::std::is_same_v<void, S>) {
        new (ret.data) S{value()};
      }
      return ret;
    }

    template<typename SS, typename = ::std::enable_if<::std::is_same_v<void, S>>>
    operator either<E, SS>() const noexcept {
      if (!is_error) {
        terminate_with("Invalid widening from a void success value.");
      }

      either<E, SS> ret{is_error};
      if constexpr (!::std::is_same_v<void, E>) {
        new (ret.data) E{error()};
      }
      return ret;
    }

    operator bool() const noexcept {
      return !is_error;
    }

    template<typename A = S, typename = ::std::enable_if_t<!::std::is_same_v<void, A>>>
    A const& value() const noexcept {
      if (is_error) {
        terminate_with("Bad either value access.");
      }
      return *reinterpret_cast<A const*>(data);
    }

    template<typename A = E, typename = ::std::enable_if_t<!::std::is_same_v<void, A>>>
    A const& error() const noexcept {
      if (!is_error) {
        terminate_with("Bad either error access.");
      }
      return *reinterpret_cast<A const*>(data);
    }

    template<typename A = S, typename = ::std::enable_if_t<!::std::is_same_v<void, A>>>
    A const& or_die() const noexcept {
      if (is_error) {
        if constexpr (!::std::is_same_v<void, E>) {
          terminate_with(error());
        } else {
          terminate_with("Method or_die called with no success value.");
        }
      }
      return value();
    }

    template<typename A = S, typename = ::std::enable_if_t<::std::is_same_v<void, A>>>
    void or_die() const noexcept {
      if (is_error) {
        if constexpr (!::std::is_same_v<void, E>) {
          terminate_with(error());
        } else {
          terminate_with("Method or_die called with no success value.");
        }
      }
    }
  };

  inline either<void, void> succeed() noexcept {
    return either<void, void>{false};
  }
 
  template<typename S>
  inline either<void, S> succeed(S s) noexcept {
    either<void, S> e{false};
    new (e.data) ::std::remove_reference_t<S>{s};
    return e;
  }    

  template<typename E>
  inline either<E> fail(E e) noexcept {
    either<E> ret{true};
    new (ret.data) ::std::remove_reference_t<E>{e};
    return ret;
  }

}
