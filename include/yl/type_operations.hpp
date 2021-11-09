  #pragma once

  #include <functional>
  #include <utility>
  #include <yl/types.hpp>

  namespace yl {

  // various ways to cast and type check

  #define DEF_CAST(type) \
    inline type& as_##type(expression& expr) noexcept { \
      return ::std::get<type>(expr); \
    } \
    inline type const& as_##type(expression const& expr) noexcept { \
      return ::std::get<type>(expr); \
    }

    DEF_CAST(numeric);
    DEF_CAST(string);
    DEF_CAST(function);
    DEF_CAST(list);
    DEF_CAST(hash_map);

  #define DEF_TYPE_CHECK(type) \
    inline bool is_##type(expression const& expr) noexcept { \
      return ::std::holds_alternative<type>(expr); \
    }

    DEF_TYPE_CHECK(numeric);
    DEF_TYPE_CHECK(string);
    DEF_TYPE_CHECK(function);
    DEF_TYPE_CHECK(list);
    DEF_TYPE_CHECK(hash_map);

  #define SUCCEED_WITH(pos, expr) \
    return succeed(make_shared<unit>(pos, expr));

  #define FAIL_WITH(msg, pos) \
    return fail(error_info{ \
      make_string(msg), \
      pos \
    });

  #define RETURN_IF_ERROR(either) \
    if (!(either)) { \
      return fail(either.error()); \
    }

    // functional style casts

  #define DEF_FUNC_CAST(type) \
    inline error_either<type> cast_##type(unit_ptr const& u_ptr) noexcept { \
      auto& [pos, expr] = *u_ptr; \
      if (is_##type(expr)) { \
        return succeed(as_##type(expr)); \
      } \
      return fail( \
        error_info{ \
          .error_message = concat("Expected " #type ", got ", type_of(expr), "."), \
          .pos = pos \
        } \
      ); \
    }

    DEF_FUNC_CAST(numeric);
    DEF_FUNC_CAST(string);
    DEF_FUNC_CAST(function);
    DEF_FUNC_CAST(list);
    DEF_FUNC_CAST(hash_map);
   
    struct identity_t {
      template<typename T>
      T operator()(T&& t) const noexcept {
        return ::std::forward<T>(t);
      }
    };

    inline auto constexpr identity = identity_t{};

    struct success_functor_t {
      template<typename T>
      auto operator()(T&& t) const noexcept {
        return succeed(::std::forward<T>(t));
      }
    };

    inline auto constexpr success_functor = success_functor_t{};

    struct fail_functor_t {
      template<typename T>
      auto operator()(T&& t) const noexcept {
        return fail(::std::forward<T>(t));
      }
    };

    inline auto constexpr fail_functor = fail_functor_t{};

    inline error_either<list> cast_q(unit_ptr const& u) noexcept {
      auto err = fail(error_info{
        concat("Expected Q expression, got ", type_of(u->expr), ", with value ", u->expr, "."),
        u->pos
      });
      return cast_list(u).collect_flat(
        [err](auto&&) { return err; },
        [err](auto&& ls) -> error_either<list> {
          if (ls.q) {
            return succeed(ls);
          }
          return err;
        }
      );
    }

    inline error_either<string> cast_r(unit_ptr const& u) noexcept {
      auto err = fail(error_info{
        concat("Expected raw string, got ", type_of(u->expr), ", with value ", u->expr, "."),
        u->pos
      });
      return cast_string(u).collect_flat(
        [err](auto&&) { return err; },
        [err](auto&& str) -> error_either<string> { 
          if (str.raw) {
            return succeed(str);
          }
          return err;
        }
      );
    }

    // get q expr or raw str
    inline error_either<either<list, string>> cast_qr(unit_ptr const& u) noexcept {
      auto err = fail(error_info{concat(
        "Expected Q expression or raw string, got ",
        type_of(u->expr),
        ", with value: ",
        u->expr,
        "."
      ), u->pos});

      using inner_t = either<list, string>;
    
      return cast_q(u).collect_flat(
        [err, u](auto&&) {
          return cast_r(u).collect_flat(
            [err](auto&&) { return err; },
            [](auto&& str) { return succeed(static_cast<inner_t>(succeed(str)));  }
          );
        },
        [](auto&& ls) { return succeed(static_cast<inner_t>(fail(ls))); }
      );
    } 

// operations on types

    inline auto len(unit_ptr const& u) noexcept {
      return cast_qr(u).or_die().collect(
        [](auto&& ls) { return ls.children.size(); },
        [](auto&& str) { return str.str.size(); }
      ).or_die();
    }

}
