#include <yl/types.hpp>
#include <yl/type_operations.hpp>

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
        out << "(";
        for (::std::size_t i = 0; i < ls.size(); ++i) {
          if (i)
            out << " ";
          out << ls[i]->expr;
        }
        out << ")";
      },
      [&out](hash_map m) {
        out << "{";
        for (auto const& [k, v] : m) {
          out << "\n"
              << "  "
              << k->expr
              << " -> "
              << v->expr;
        }
        if (m.size()) {
          out << "\n";
        }
        out << "}";
      }
    }, e);
    return out;
  }
  
  ::std::string type_of(expression const& e) noexcept {
    return ::std::visit(overloaded {
      [](numeric) { return "numeric"; },
      [](string) { return "string"; },
      [](function) { return "function"; },
      [](list) { return "list"; },
      [](hash_map) { return "map"; }
    }, e);
  }

  bool operator==(unit_ptr const& a, unit_ptr const& b) noexcept {
    if (a.get() == b.get()) {
      return true;
    }

    if (a->expr.index() != b->expr.index()) {
      return false;
    }

    if (is_numeric(a->expr)) {
      return as_numeric(a->expr) == as_numeric(b->expr);
    }

    if (is_string(a->expr)) {
      auto const& sa = as_string(a->expr);
      auto const& sb = as_string(b->expr);
      return sa.raw == sb.raw && sa.str == sb.str;
    }

    if (is_list(a->expr)) {
      auto const& al = as_list(a->expr);
      auto const& bl = as_list(b->expr);
      return al == bl;
    }

    if (is_hash_map(a->expr)) {
      auto const& ha = as_hash_map(a->expr);
      auto const& hb = as_hash_map(b->expr);

      return ha == hb;
    }
    
    return false;
  }

  ::std::size_t unit_hasher::operator()(unit_ptr const& u) const noexcept {
    return ::std::visit(overloaded {
      [](numeric n) { 
        return ::std::hash<numeric>{}(n); 
      },
      [](string s) { 
        // TODO: combine these two properly
        return 
          ::std::hash<string_representation>{}(s.str) 
            ^ ::std::hash<bool>{}(s.raw); 
      },
      [](function) {
        return ::std::size_t{0}; 
      },
      [](list ls) {
        // TODO:
        ::std::size_t ret{};
        auto hasher = unit_hasher{};
        for (auto&& child : ls) {
          ret ^= hasher(child);
        }
        return ret;
      },
      [](hash_map hm) { 
        // TODO:
        ::std::size_t ret = 0;
        auto hasher = unit_hasher{};
        for (auto const& [k, v] : hm) {
          ret ^= (hasher(k) << 7) ^ (hasher(v));
        }
        return ret;
      }
    }, u->expr);
  }

}
