#pragma once

#include <functional>
#include <type_traits>

#include <yl/util.hpp>

namespace yl {

  template<typename E, typename S>
  class either;

  template<typename S, typename SS = ::std::remove_reference_t<S>>
  [[nodiscard]] inline either<void, SS> succeed(S s) noexcept;

  [[nodiscard]] inline either<void, void> succeed() noexcept;


  namespace detail {

    template<typename T>
    struct custom_type_info {
      auto constexpr static size = sizeof(T);
      auto constexpr static align = alignof(T);
    };

    template<>
    struct custom_type_info<void> {
      auto constexpr static size = sizeof(char);
      auto constexpr static align = alignof(char);
    };

    template<typename T>
    using custom_type_info_t = custom_type_info<T>;

    template<typename F, typename T>
    constexpr auto get_invoke_result() noexcept {
      if constexpr (::std::is_same_v<T, void>) {
        return ::std::invoke_result<F>{};
      } else {
        return ::std::invoke_result<F, T>{};
      }
    }

    template<typename F, typename T>
    using invoke_result_t = typename decltype(get_invoke_result<F, T>())::type;

    template<typename U, typename V>
    struct common_either;

    template<typename E, typename S>
    struct common_either<either<E, S>, either<E, S>> {
      using type = either<E, S>;
    };

    template<typename E>
    struct common_either<either<E, void>, either<E, void>> {
      using type = either<E, void>;
    };

    template<typename S>
    struct common_either<either<void, S>, either<void, S>> {
      using type = either<void, S>;
    };

    template<typename E, typename S>
    struct common_either<either<E, void>, either<void, S>> {
      using type = either<E, S>;
    };

    template<typename E, typename S>
    struct common_either<either<E, void>, either<E, S>> {
      using type = either<E, S>;
    };

    template<typename E, typename S>
    struct common_either<either<void, S>, either<E, void>> {
      using type = either<E, S>;
    };

    template<typename E, typename S>
    struct common_either<either<E, S>, either<void, S>> {
      using type = either<E, S>;
    };

    template<typename E, typename S>
    struct common_either<either<E, S>, either<E, void>> {
      using type = either<E, S>;
    };

    template<typename... Ts>
    using common_either_t = typename common_either<Ts...>::type;

  }

  template<typename E, typename S = void>
  class either {
    using ti_E = detail::custom_type_info_t<E>;
    using ti_S = detail::custom_type_info_t<S>;

    auto constexpr static E_not_void = !::std::is_same_v<void, E>;
    auto constexpr static S_not_void = !::std::is_same_v<void, S>;

   public:
    using error_type = E;
    using success_type = S;

    // the price to pay
    alignas(::std::max(ti_E::align, ti_S::align)) 
      char data[::std::max(ti_E::size, ti_S::size)];

    bool err;

    explicit either(bool const is_error = true) noexcept 
      : err(is_error) {}

    bool is_error() const noexcept {
      return err;
    }

    operator bool() const noexcept {
      return !is_error();
    }

    ~either() {
      if (is_error()) {
        if constexpr (E_not_void) {
          (reinterpret_cast<E&>(data)).~E();
        }
      } else {
        if constexpr (S_not_void) {
          (reinterpret_cast<S&>(data)).~S();
        }
      }
    }

    either(either const& other) noexcept { 
      *this = other;
    }

    either& operator=(either const& other) noexcept {
      if (other) {
        if constexpr (S_not_void) {
          new (data) S(other.value());
        }
      } else {
        if constexpr (E_not_void) {
          new (data) E(other.error());
        }
      }
      err = other.err;
      return *this;
    }

    either(either&& other) noexcept {
      *this = ::std::forward<either>(other);
    }

    either& operator=(either&& other) noexcept {
      if (other) {
        if constexpr (S_not_void) {
          new (data) S(::std::move(reinterpret_cast<S&>(other.data)));
        }
      } else {
        if constexpr (E_not_void) {
          new (data) E(::std::move(reinterpret_cast<E&>(other.data)));
        }
      }
      err = other.err;
      return *this;
    }

    template<typename EE, typename OE = E, typename = ::std::enable_if_t<::std::is_same_v<void, OE>>>
    operator either<EE, S>() const noexcept {
      if (is_error()) {
        terminate_with("Invalid widening from an error value.");
      }  
  
      either<EE, S> ret{is_error()};
      if constexpr (S_not_void) {
        new (ret.data) S{value()};
      }
      return ret;
    }

    template<typename SS, typename OS = S, typename = ::std::enable_if_t<::std::is_same_v<void, OS>>>
    operator either<E, SS>() const noexcept {
      if (!is_error()) {
        terminate_with("Invalid widening from a success value.");
      }

      either<E, SS> ret{is_error()};
      if constexpr (E_not_void) {
        new (ret.data) E{error()};
      }
      return ret;
    }

    template<typename A = S, typename = ::std::enable_if_t<!::std::is_same_v<void, A>>>
    A const& value() const noexcept {
      if (is_error()) {
        terminate_with("Bad either value access.");
      }
      return reinterpret_cast<A const&>(data);
    }

    template<typename A = S, typename = ::std::enable_if_t<!::std::is_same_v<void, A>>>
    A const& operator*() const noexcept {
      return value();
    }

    template<typename A = E, typename = ::std::enable_if_t<!::std::is_same_v<void, A>>>
    A const& error() const noexcept {
      if (!is_error()) {
        terminate_with("Bad either error access.");
      }
      return reinterpret_cast<A const&>(data);
    }

    template<typename A = S, typename = ::std::enable_if_t<!::std::is_same_v<void, A>>>
    A const& or_die() const noexcept {
      if (is_error()) {
        if constexpr (E_not_void) {
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
        if constexpr (E_not_void) {
          terminate_with(error());
        } else {
          terminate_with("Method or_die called with no success value.");
        }
      }
    }

    // quality of life additions
    // maps
    template<
      typename F, 
      typename SS = S, 
      typename NewS = ::std::invoke_result_t<F, SS>
    >
    [[nodiscard]] ::std::enable_if_t<!::std::is_same_v<void, SS>, either<E, NewS>> 
    map(F&& f) const noexcept {
      if (!is_error()) {
        if constexpr(!::std::is_same_v<void, NewS>) {
          return succeed(f(this->value()));
        } else {
          f(this->value());
          return succeed();
        }
      }
      return *this;
    }

    template<
      typename F, 
      typename SS = S, 
      typename NewS = ::std::invoke_result_t<F>
    >
    [[nodiscard]] either<E, NewS> map(F&& f) const noexcept {
      if (!is_error()) {
        if constexpr(!::std::is_same_v<void, NewS>) {
          return succeed(f());
        } else {
          f();
          return succeed();
        }
      }
      return *this;
    }

    // flatmaps
    template<
      typename F, 
      typename EE = E,
      typename SS = S, 
      typename NewS = detail::common_either_t<::std::invoke_result_t<F, SS>, either<EE, void>>
    >
    [[nodiscard]] ::std::enable_if_t<!::std::is_same_v<void, SS>, NewS> 
    flat_map(F&& f) const noexcept {
      if (!is_error()) {
        return f(this->value());
      }
      return fail(this->error());
    }

    template<
      typename F, 
      typename EE = E,
      typename SS = S, 
      typename NewS = detail::common_either_t<::std::invoke_result_t<F>, either<EE, void>>
    >
    [[nodiscard]] NewS flat_map(F&& f) const noexcept {
      if (!is_error()) {
        return f();
      }
      return fail(this->error());
    }

    // TODO: add for common types. not just either results?
    template<
      typename F1,
      typename F2,
      typename R1 = detail::invoke_result_t<F1, E>,
      typename R2 = detail::invoke_result_t<F2, S>,
      typename C  = detail::common_either_t<R1, R2>
    >
    [[nodiscard]] C collect_flat(F1&& f1, F2&& f2) const noexcept {
      if (is_error()) {
        if constexpr (::std::is_same_v<void, E>) {
          return f1();
        } else {
          return f1(this->error());
        }
      }
      if constexpr (::std::is_same_v<void, S>) {
        return f2();
      } else {
        return f2(this->value());
      }
    }

    
    template<
      typename F1,
      typename F2,
      typename R1 = detail::invoke_result_t<F1, E>,
      typename R2 = detail::invoke_result_t<F2, S>,
      typename C  = ::std::common_type_t<R1, R2>
    >
    [[nodiscard]] either<E, C> collect(F1&& f1, F2&& f2) const noexcept {
      if (is_error()) {
        if constexpr (::std::is_same_v<void, E>) {
          return succeed(static_cast<C>(f1()));
        } else {
          return succeed(static_cast<C>(f1(this->error())));
        }
      }
      if constexpr (::std::is_same_v<void, S>) {
        return succeed(static_cast<C>(f2()));
      } else {
        return succeed(static_cast<C>(f2(this->value())));
      }
    }
  };

  [[nodiscard]] inline either<void, void> succeed() noexcept {
    return either<void, void>{false};
  }
 
  template<typename S, typename SS>
  [[nodiscard]] inline either<void, SS> succeed(S s) noexcept {
    either<void, SS> e{false};
    new (e.data) SS{::std::move(s)};
    return e;
  }    

  template<typename E, typename EE = ::std::remove_reference_t<E>>
  [[nodiscard]] inline either<EE> fail(E e) noexcept {
    either<EE> ret{true};
    new (ret.data) EE{e};
    return ret;
  } 

  template<
    typename E, typename S, typename SS,
    typename = ::std::enable_if_t<!::std::is_same_v<void, SS>>
  >
  inline either<E, SS> map_to(
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
