#pragma once

#include <yl/types.hpp>

namespace yl {

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

#define DEF_TYPE_CHECK(type) \
  inline bool is_##type(expression const& expr) noexcept { \
    return ::std::holds_alternative<type>(expr); \
  }

  DEF_TYPE_CHECK(numeric);
  DEF_TYPE_CHECK(string);
  DEF_TYPE_CHECK(function);
  DEF_TYPE_CHECK(list);

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

}
