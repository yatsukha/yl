#include "yl/either.hpp"
#include "yl/parser.hpp"
#include "yl/util.hpp"
#include <stdexcept>
#include <string>
#include <functional>
#include <unordered_map>
#include <cmath>
#include <yl/eval_polish.hpp>

namespace yl {

  either<error_info, double> eval(poly_base const& expr) noexcept {
    using operation = ::std::function<double(double, double)>;

    ::std::unordered_map<::std::string, operation> static ops{
      {"+", [](auto&& a, auto&& b) { return a + b; }},
      {"-", [](auto&& a, auto&& b) { return a - b; }},
      {"/", [](auto&& a, auto&& b) { return a / b; }},
      {"*", [](auto&& a, auto&& b) { return a * b; }},
      {"**", [](auto&& a, auto&& b) { return ::std::pow(a, b); }},
    };

    if (!expr) {
      return fail(error_info{"Can not evaluate empty expression.", 0});
    }

    if (auto t = dynamic_cast<terminal const*>(expr.get()); t) {
      try {
        return succeed(::std::stod(t->data));
      } catch (::std::invalid_argument const& exc) {
        return fail(error_info{"Can not convert to a number.", t->start});
      }
    } else {
      auto e = dynamic_cast<expression const*>(expr.get());

      if (e->args.empty()) {
        return fail(error_info{"Can not evaluate empty expression.", e->start});
      } else if (e->args.size() < 3) {
        if (e->args.size() == 1) {
          return eval(e->args.front());
        } else {
          return fail(error_info{
            "Expression must be either a unit, or an n-ary expression with atleast 2 operands.\n"
            "  unit: (+ 2 3)\n"
            "  n-ary: + 2 3 4 5 (+ 2 3)",
            e->start
          });
        }
      }

      if (auto t = dynamic_cast<terminal const*>(e->args.front().get()); t) {
        auto const& op = t->data;
        if (!ops.count(op)) {
          return fail(error_info{"Unknown operator.", t->start});
        }

        auto operation = ops[op];
        auto current = eval(e->args[1]);

        if (!current) {
          return current;
        }

        auto running_result = current.value();

        for (::std::size_t i = 2; i < e->args.size(); ++i) {
          current = eval(e->args[i]);
          if (!current) {
            return current;
          }

          running_result = operation(running_result, current.value());
        }

        return succeed(running_result);
      } else {
        return fail(error_info{"Expected a token.", e->args.front()->start});
      }
    }
  }

}
