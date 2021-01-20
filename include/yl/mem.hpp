#pragma once

#include <memory>
#include <memory_resource>
#include <string>
#include <utility>

namespace yl {

  using string_representation = ::std::pmr::string;

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

  namespace detail {

    template<typename T>
    using pmr_alloc_t = ::std::pmr::polymorphic_allocator<T>;

  }

  ::std::pmr::unsynchronized_pool_resource inline mem_pool{};

  template<typename T>
  ::std::shared_ptr<T> make_shared(T&& t) noexcept {
    return ::std::allocate_shared<T, detail::pmr_alloc_t<T>>(
      &mem_pool, ::std::forward<T>(t)
    ); 
  }
  
  template<typename T, typename... Args>
  ::std::shared_ptr<T> make_shared(Args&& ...args) noexcept {
    auto ret = ::std::allocate_shared<T, detail::pmr_alloc_t<T>>(
      &mem_pool
    );
    new (ret.get()) T{::std::forward<Args>(args)...}; // smelly
    return ret;
  }

}
