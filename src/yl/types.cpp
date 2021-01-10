#include <yl/types.hpp>

namespace yl {

  ::std::ostream& operator<<(::std::ostream& out, expression const& e) noexcept {
    ::std::visit(overloaded {
      [&out](numeric any) { out << any; },
      [&out](string any) { 
        if (any.raw) {
          out << "\"";
        }
        out << any.str; 
        if (any.raw) {
          out << "\"";
        }
      },
      [&out](function fn) { out << fn.description; },
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
  
  ::std::string type_of(expression const& e) noexcept {
    ::std::string s;
    ::std::visit(overloaded {
      [&s](numeric) { s = "numeric"; },
      [&s](string) { s = "symbol"; },
      [&s](function) { s = "function"; },
      [&](list) { s = "list"; },
    }, e);
    return s;
  }

}
