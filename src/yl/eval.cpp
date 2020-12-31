#include "yl/either.hpp"
#include "yl/parse.hpp"
#include "yl/util.hpp"
#include <stdexcept>
#include <string>
#include <functional>
#include <unordered_map>
#include <cmath>
#include <yl/eval.hpp>

namespace yl {

  eval_either eval_s(poly_base& expr) noexcept {
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
        return succeed(static_cast<result_type>(succeed(::std::stod(t->data))));
      } catch (::std::invalid_argument const& exc) {
        return fail(error_info{"Can not convert to a number.", t->start});
      }
    } else {
      auto e = dynamic_cast<expression*>(expr.get());

      if (e->args.empty() || e->q) {
        return succeed(static_cast<result_type>(fail(expr->copy())));
      } else if (e->args.size() < 2) {
        return eval_s(e->args.front());
      }

      if (auto t = dynamic_cast<terminal const*>(e->args.front().get()); t) {
        auto const& op = t->data;
        if (!ops.count(op)) {
          return fail(error_info{"Unknown operator.", t->start});
        }

        auto operation = ops[op];

        for (auto&& arg : e->args) {
          if (auto as_expr = dynamic_cast<expression*>(arg.get()); as_expr) {
            if (as_expr->q) {
              return succeed(static_cast<result_type>(fail(expr)));
            }
          }
        }

        auto current = eval_s(e->args[1]);

        if (!current) {
          return current;
        } else if (!current.value()) {
          return succeed(static_cast<result_type>(fail(expr->copy())));
        }

        auto running_result = current.value().value();

        for (::std::size_t i = 2; i < e->args.size(); ++i) {
          current = eval_s(e->args[i]);
          if (!current) {
            return current;
          } else if (!current.value()) {
            return succeed(static_cast<result_type>(fail(expr->copy())));
          }

          running_result = operation(running_result, current.value().value());
        }

        return succeed(static_cast<result_type>(succeed(running_result)));
      } else {
        return fail(error_info{"Expected a token.", e->args.front()->start});
      }
    }
  }

  eval_either eval(poly_base& expr) noexcept {
    return eval_s(expr);
  }

}
