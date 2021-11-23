#include <yl/traits/base.hpp>

namespace yl::traits {

  bool base::operator==(base_ptr const&) const noexcept {
    return false;
  };

  ::std::size_t base::hash() const noexcept {
    return 0ul;
  }

}
