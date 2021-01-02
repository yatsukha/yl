#include <yl/types.hpp>

namespace yl {

  ::std::ostream& operator<<(::std::ostream& out, expression const& e) noexcept {
    ::std::visit(overloaded {
      [&out](numeric any) { out << any; },
      [&out](symbol any) { out << any; },
      [&out](function fn) { out << "<fn>: " << fn.description; },
      [&out](ls list) {
        out << (list.q ? "{" : "(");
        for (::std::size_t i = 0; i < list.children.size(); ++i) {
          if (i)
            out << " ";
          out << list.children[i].expr;
        }
        out << (list.q ? "}" : ")");
      }
    }, e);
    return out;
  }


}
