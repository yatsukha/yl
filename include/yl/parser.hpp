#pragma once

#include <memory>
#include <ostream>
#include <vector>

#include <yl/either.hpp>

namespace yl {

  struct base;

  using poly_base = ::std::shared_ptr<base>;

  struct base {
    ::std::size_t start;

    base(::std::size_t const start) noexcept : start(start) {}

    virtual void print(::std::ostream& out) const noexcept = 0;
    virtual poly_base copy() const noexcept = 0;

    virtual ~base() = default;
  };


  struct terminal : base {
    ::std::string data;

    terminal(::std::size_t const start, ::std::string const data) noexcept
      : base(start), data(data) {}

    virtual void print(::std::ostream& out) const noexcept override {
      out << data;
    }

    virtual poly_base copy() const noexcept override {
      auto other = new terminal{start, data};
      return poly_base{other};
    }
  };

  struct expression : base {
    ::std::vector<poly_base> args;
    
    expression(::std::size_t const start) noexcept : base(start) {}

    virtual void print(::std::ostream& out) const noexcept override {
      out << "(";
      for (::std::size_t i = 0; i < args.size(); ++i) {
        if (i) {
          out << " ";
        }
        args[i]->print(out);
      }
      out << ")";
    }

    virtual poly_base copy() const noexcept override {
      auto other = new expression{start};
      other->args.reserve(args.size());
      for (auto&& arg : args) {
        other->args.push_back(arg->copy());
      }
      return poly_base{other};
    }
  };

  struct error_info {
    char const* error_message;
    ::std::size_t const column;
  };

  using parse_result = either<error_info, poly_base>;

  parse_result parse_polish(char const* line) noexcept;

}
