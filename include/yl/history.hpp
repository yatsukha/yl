#pragma once

#include <cstdint>

#include <yl/mem.hpp>

namespace yl {

  class history {
#ifdef __EMSCRIPTEN__
    seq_representation<string_representation> static lines;
#endif

   public:
    static void append(string_representation const&) noexcept;
    static string_representation get(::std::size_t const) noexcept;
    static ::std::size_t size() noexcept;
  };

}
