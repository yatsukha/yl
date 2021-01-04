#pragma once

#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <yl/util.hpp>
#include <type_traits>
#include <utility>

namespace yl {

  template<typename E, typename S = void>
  class either {
    using _E = ::std::conditional_t<::std::is_same_v<void, E>, char, E>;  
    auto constexpr static sz_e = sizeof(_E);
    using _S = ::std::conditional_t<::std::is_same_v<void, S>, char, S>;
    auto constexpr static sz_s = sizeof(_S);
    auto constexpr static max_sz = ::std::max(sz_e, sz_s);

    auto constexpr static alignment = ::std::max(alignof(_E), alignof(_S));
    auto constexpr static actual_sz = max_sz + alignment;

   public:
    using error_type = E;
    using success_type = S;

    // the price to pay
    // alignas(::std::max(alignof(_E), alignof(_S))) char data[max_sz];

    ::std::shared_ptr<char[]> data{
      reinterpret_cast<char*>(::aligned_alloc(alignment, actual_sz << 1)),
      [sz = actual_sz](auto* ptr) {
        char* data = reinterpret_cast<char*>(ptr);

        if (data[sz]) {
          if constexpr (!::std::is_same_v<void, E>) {
            (*reinterpret_cast<E*>(data)).~E();
          }
        } else {
          if constexpr (!::std::is_same_v<void, S>) {
            (*reinterpret_cast<S*>(data)).~S();
          }
        }

        ::free(ptr);
      }
    };

    explicit either(bool const is_error = true) noexcept {
      data[actual_sz] = is_error;
    }

    bool is_error() const noexcept {
      return data[actual_sz] & 0b1;
    }

    operator bool() const noexcept {
      return data[actual_sz] ^ 0b1;
    }

    /*~either() {
      if (state & 0b10) {
        return;
      }

      if (is_error()) {
        if constexpr (!::std::is_same_v<void, E>) {
          (*reinterpret_cast<E*>(data)).~E();
        }
      } else {
        if constexpr (!::std::is_same_v<void, S>) {
          (*reinterpret_cast<S*>(data)).~S();
        }
      }
    }

    either(either const& other) noexcept { 
      if (other) {
        if constexpr (!::std::is_same_v<void, S>) {
          new (data) S(other.value());

        }
      } else {
        if constexpr (!::std::is_same_v<void, E>) {
          new (data) E(other.error());
        }
      }
      state = other.state;
      state &= 0b01;
    }

    either& operator=(either const& other) noexcept {
      if (other) {
        if constexpr (!::std::is_same_v<void, S>) {
          new (data) S(other.value());
        }
      } else {
        if constexpr (!::std::is_same_v<void, E>) {
          new (data) E(other.error());
        }
      }
      state = other.state;
      state &= 0b01;
      return *this;
    }

    either(either&& other) noexcept {
      if (other) {
        if constexpr (!::std::is_same_v<void, S>) {
          new (data) S(other.value());

        }
      } else {
        if constexpr (!::std::is_same_v<void, E>) {
          new (data) E(other.error());
        }
      }
      state = other.state;
      other.state |= 0b10;
      state &= 0b01;
    }

    either& operator=(either&& other) noexcept {
      if (other) {
        if constexpr (!::std::is_same_v<void, S>) {
          new (data) S(other.value());
        }
      } else {
        if constexpr (!::std::is_same_v<void, E>) {
          new (data) E(other.error());
        }
      }
      state = other.state;
      other.state |= 0b10;
      state &= 0b01;
      return *this;
    }*/

    template<typename EE>
    operator either<EE, S>() const noexcept {
      if (is_error()) {
        terminate_with("Invalid widening from a void error value.");
      }  
  
      either<EE, S> ret{is_error()};
      if constexpr (!::std::is_same_v<void, S>) {
        new (ret.data.get()) S{value()};
      }
      return ret;
    }

    template<typename SS>
    operator either<E, SS>() const noexcept {
      if (!is_error()) {
        terminate_with("Invalid widening from a void success value.");
      }

      either<E, SS> ret{is_error()};
      if constexpr (!::std::is_same_v<void, E>) {
        new (ret.data.get()) E{error()};
      }
      return ret;
    }

    template<typename A = S, typename = ::std::enable_if_t<!::std::is_same_v<void, A>>>
    A const& value() const noexcept {
      if (is_error()) {
        terminate_with("Bad either value access.");
      }
      return *reinterpret_cast<A const*>(data.get());
    }

    template<typename A = E, typename = ::std::enable_if_t<!::std::is_same_v<void, A>>>
    A const& error() const noexcept {
      if (!is_error()) {
        terminate_with("Bad either error access.");
      }
      return *reinterpret_cast<A const*>(data.get());
    }

    template<typename A = S, typename = ::std::enable_if_t<!::std::is_same_v<void, A>>>
    A const& or_die() const noexcept {
      if (is_error()) {
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
      if (is_error()) {
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
 
  template<typename S, typename SS = ::std::remove_reference_t<S>>
  inline either<void, SS> succeed(S s) noexcept {
    either<void, SS> e{false};
    new (e.data.get()) SS{s};
    return e;
  }    

  template<typename E, typename EE = ::std::remove_reference_t<E>>
  inline either<EE> fail(E e) noexcept {
    either<EE> ret{true};
    new (ret.data.get()) EE{e};
    return ret;
  } 

  template<
    typename E, typename S, typename SS,
    typename = ::std::enable_if_t<!::std::is_same_v<void, SS>>
  >
  inline either<E, SS> map(
    either<E, S> const& e,
    ::std::function<SS(S const&)> const& f
  ) noexcept {
    if (!e) {
      return e;
    }
    return succeed(f(e.value()));
  }

  template<typename E, typename S1, typename S2>
  inline either<E, ::std::pair<S1, S2>> aggregate(
    either<E, S1> const& e1,
    either<E, S2> const& e2
  ) noexcept {
    if (!e1) return fail(e1.error());
    if (!e2) return fail(e2.error());

    return succeed(::std::pair<S1, S2>{e1.value(), e2.value()});
  }

  template<typename E, typename S>
  inline either<E, S> any(
    either<E, S> const& u,
    either<E, S> const& v
    ) noexcept {
    if (u) return u;
    if (v) return v;
    return fail(u.error());
  }

}