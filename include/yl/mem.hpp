#pragma once

#include <memory>
#include <vector>
#ifndef __EMSCRIPTEN__ 
#include <memory_resource>
#endif
#include <string>
#include <utility>

#ifndef __EMSCRIPTEN__
#define PMR_PREF ::std::pmr
#else
#define PMR_PREF ::std
#endif

namespace yl {

#ifndef __EMSCRIPTEN__
  ::std::pmr::unsynchronized_pool_resource inline mem_pool{};
#endif

  using string_representation = PMR_PREF::string;

  inline string_representation make_string() noexcept {
    return string_representation{
#ifndef __EMSCRIPTEN__
      &mem_pool
#endif
    };
  }

  inline string_representation make_string(char const* charr) noexcept {
    return string_representation{
      charr
#ifndef __EMSCRIPTEN__
      , &mem_pool
#endif
    };
  }

  inline string_representation make_string(string_representation const& str) noexcept {
    return str;
  }

  template<typename Iter>
  inline string_representation make_string(Iter begin, Iter end) noexcept {
    return string_representation{
      begin, end
#ifndef __EMSCRIPTEN__
      , &mem_pool
#endif
    };
  }

  template<typename T>
  using seq_representation = PMR_PREF::vector<T>;

  template<typename T>
  inline seq_representation<T> make_seq() noexcept {
    return seq_representation<T>{  
#ifndef __EMSCRIPTEN__
      &mem_pool
#endif
    };
  }

  template<typename T, typename Iter>
  inline seq_representation<T> make_seq(Iter begin, Iter end) noexcept {
    return seq_representation<T>{
      begin, end
#ifndef __EMSCRIPTEN__
      , &mem_pool
#endif
    };
  }

  // gotta go fast, even if without brain
  struct str_hasher {
    ::std::size_t operator()(string_representation const& str) const noexcept {
      switch (str.size()) {
        case 0:
          return 0;
        case 1:
          return str[0];
        default:
          return str[0] ^ str[1];
      }
    }
  };

#ifndef __EMSCRIPTEN__
  namespace detail {

    template<typename T>
    using pmr_alloc_t = ::std::pmr::polymorphic_allocator<T>;

  }
#endif


  template<typename T>
  ::std::shared_ptr<T> make_shared(T&& t) noexcept {
#ifndef __EMSCRIPTEN__
    return ::std::allocate_shared<T, detail::pmr_alloc_t<T>>(
      &mem_pool, ::std::forward<T>(t)
    );
#else
    return ::std::make_shared<T>(::std::forward<T>(t));
#endif
  }
  
  template<typename T, typename... Args>
  ::std::shared_ptr<T> make_shared(Args&& ...args) noexcept {
#ifndef __EMSCRIPTEN__
    auto ret = ::std::allocate_shared<T, detail::pmr_alloc_t<T>>(
      &mem_pool
    );
#else
    auto ret = ::std::make_shared<T>();
#endif
    new (ret.get()) T{::std::forward<Args>(args)...}; // smelly
    return ret;
  }

}
