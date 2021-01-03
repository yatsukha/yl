#include <yl/types.hpp>

namespace yl {

  ::std::ostream& operator<<(::std::ostream& out, expression const& e) noexcept {
    ::std::visit(overloaded {
      [&out](numeric any) { out << any; },
      [&out](symbol any) { out << any; },
      [&out](function fn) { out << "<fn>: " << fn.description; },
      [&out](list ls) {
        out << (ls.q ? "{" : "(");
        for (::std::size_t i = 0; i < ls.children.size(); ++i) {
          if (i)
            out << " ";
          out << ls.children[i].expr;
        }
        out << (ls.q ? "}" : ")");
      }
    }, e);
    return out;
  }


}
