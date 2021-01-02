#include "yl/types.hpp"
#include <unordered_map>
#include <cmath>

#include <variant>
#include <yl/eval.hpp>

namespace yl {


/*  eval_either eval_s(poly_base&) noexcept;

  namespace operations {

#define SUCCEED_WITH_NUMBER(n) \
    return succeed(static_cast<result_type>(succeed(n)));

#define SUCCEED_WITH_EXPRESSION(expr) \
    return succeed(static_cast<result_type>(fail(expr)));

#define RETURN_ERROR_IF_EMPTY(args) \
    if (args.size() <= 1) { \
      return fail(error_info{"Operator expects atleast 1 operand.", args.front()->start});\
    } 

#define RETURN_IF_ERROR(either) \
  if (!either) {\
    return either; \
  }

#define RETURN_IF_NOT_NUMBER(variant, idx) \
  if (!::std::holds_alternative<double>(variant)) { \
    return fail(error_info{ \
      "Expected a number.", \
      idx \
    }); \
  }

#define RETURN_IF_NOT_EXPR(either, idx) \
  if (!::std::holds_alternative<poly_base>(either.value())) { \
    return fail(error_info{ \
      "Expected an expression.", \
      idx \
    }); \
  } \
  if (!dynamic_cast<expression const*>( \
        ::std::get<poly_base>(either.value()))) { \
    return fail(error_info{\
      "Expected an expression.", \
      idx \
    }); \
  }

#define RETURN_ERROR_IF_Q(either) \
  if (either && !either.value()) {\
    return fail(error_info{ \
      "Can not use an unevaluated Q expression in this context.\n" \
      "Did you forgot to use 'eval'?", \
      either.value().error()->start \
    }); \
  }

#define DEFINE_N_ARY_OPERATOR(name, operation) \
    eval_either name(operands args) noexcept { \
      RETURN_ERROR_IF_EMPTY(args); \
      double running_total; \
      for (::std::size_t i = 1; i < args.size(); ++i) { \
        auto evaluated = eval_s(args[i]); \
        RETURN_IF_ERROR(evaluated); \
        RETURN_IF_NOT_NUMBER(evaluated.value(), args[i]->start); \
        auto const value = ::std::get<double>(evaluated.value()); \
        if (i == 1) { \
          running_total = value; \
        } else { \
          running_total operation##= value; \
        } \
      } \
      SUCCEED_WITH_NUMBER(running_total); \
    }

#define DEFINE_MACRO(name, ...) \
    eval_either name(operands args) noexcept { \
      __VA_ARGS__ \
    }

    DEFINE_N_ARY_OPERATOR(add, +);
    DEFINE_N_ARY_OPERATOR(sub, -);
    DEFINE_N_ARY_OPERATOR(mul, *);
    DEFINE_N_ARY_OPERATOR(div, /);

    DEFINE_MACRO(eval_m, {
      if (args.size() > 2) {
        fail(error_info{
          "Too many expressions, expected a single Q expression.", args[2]->start
        });
      }

      auto operand = args[1]->copy();

      if (auto ptr = dynamic_cast<expression*>(operand.get()); ptr) {
        if (!ptr->q) {
          auto tmp_eval = eval_s(operand);
          RETURN_IF_ERROR(tmp_eval);

          if (tmp_eval.value()) {
            return fail(error_info{
              "Expected a Q expression, got a value.", operand->start
            }); 
          }

          operand = tmp_eval.value().error()->copy();
          ptr = dynamic_cast<expression*>(operand.get());
        }

        ptr->q = false;
        return eval_s(operand);
      } else {
        return fail(error_info{"Expected a Q expression.", operand->start});
      }
    });

    DEFINE_MACRO(list_m, {
      RETURN_ERROR_IF_EMPTY(args);

      auto q_expr = new expression{args[0]->start, true};
      q_expr->args.reserve(args.size() - 1);
      for (::std::size_t i = 1; i < args.size(); ++i) {
        q_expr->args.push_back(args[i]);
      }

      SUCCEED_WITH_EXPRESSION(poly_base{q_expr});
    });

    DEFINE_MACRO(head_m, {
      RETURN_ERROR_IF_EMPTY(args);

      auto operand = args[1]->copy();

      if (auto ptr = dynamic_cast<expression*>(operand.get()); ptr) {
        if (!ptr->q) {
          auto tmp_eval = eval_s(operand);
          RETURN_IF_ERROR(tmp_eval);

          if (tmp_eval.value()) {
            return fail(error_info{
              "Expected a Q expression, got a value.", operand->start
            }); 
          }

          operand = tmp_eval.value().error()->copy();
          ptr = dynamic_cast<expression*>(operand.get());
        }
       
        if (ptr->args.empty()) {
          return fail(error_info{"Expected at leat one element.", ptr->start});
        }
        
        auto expr = new expression{ptr->args[0]->start, true};
        expr->args.push_back(ptr->args[0]);
        SUCCEED_WITH_EXPRESSION(poly_base{expr});
      } else {
        return fail(error_info{"Expected a Q expression.", operand->start});
      }
    });

    DEFINE_MACRO(tail_m, {
      RETURN_ERROR_IF_EMPTY(args);

      auto operand = args[1]->copy();

      if (auto ptr = dynamic_cast<expression*>(operand.get()); ptr) {
        if (!ptr->q) {
          auto tmp_eval = eval_s(operand);
          RETURN_IF_ERROR(tmp_eval);

          if (tmp_eval.value()) {
            return fail(error_info{
              "Expected a Q expression, got a value.", operand->start
            }); 
          }

          operand = tmp_eval.value().error()->copy();
          ptr = dynamic_cast<expression*>(operand.get());
        }
        
        
        auto expr = new expression{ptr->args[0]->start, true};
        expr->args.reserve(ptr->args.size() - 2);
        for (::std::size_t i = 1; i < ptr->args.size(); ++i) {
          expr->args.push_back(ptr->args[i]);
        }
        SUCCEED_WITH_EXPRESSION(poly_base{expr});
      } else {
        return fail(error_info{"Expected a Q expression.", operand->start});
      }
    });

    DEFINE_MACRO(join_m, {
      RETURN_ERROR_IF_EMPTY(args);

      auto expr = new expression{args[1]->start, true};
      expr->args.reserve(args.size() - 1);

      auto ret = poly_base{expr};

      for (::std::size_t i = 1; i < args.size(); ++i) {
        auto operand = args[i]->copy();
        if (auto ptr = dynamic_cast<expression*>(operand.get()); ptr) {
          if (!ptr->q) {
            auto tmp_eval = eval_s(operand);
            RETURN_IF_ERROR(tmp_eval);

            if (tmp_eval.value()) {
              return fail(error_info{
                "Expected a Q expression, got a value.", operand->start
              }); 
            }

            operand = tmp_eval.value().error()->copy();
          }
          expr->args.push_back(operand);
        } else {
          return fail(error_info{"Expected a Q expression.", operand->start});
        }
      }

      SUCCEED_WITH_EXPRESSION(ret);
    });

    inline ::std::unordered_map<::std::string, operation> ops{
      {"+", add},
      {"-", sub},
      {"*", mul},
      {"/", div},
      {"eval", eval_m},
      {"list", list_m},
      {"head", head_m},
      {"tail", tail_m},
      {"join", join_m},
    };

  }

  eval_either eval_s(poly_base& expr) noexcept {
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
        if (!operations::ops.count(op)) {
          return fail(error_info{"Unknown operator.", t->start});
        }

        auto operation = operations::ops[op];

        return operation(e->args);
      } else {
        return fail(error_info{"Expected a token.", e->args.front()->start});
      }
    }
  }

  eval_either eval(poly_base& expr) noexcept {
    return eval_s(expr);
  } */

  /*
   *
   * BUILTIN OPERATIONS
   *
   */

  either<error_info, numeric> numeric_or_error(unit u) noexcept {
    auto ret = eval(u);

    if (!ret) {
      return fail(ret.error());
    }

    u = ret.value();

    auto const& [idx, expr] = u;
    
    if (::std::holds_alternative<numeric>(expr)) {
      return succeed(::std::get<numeric>(expr));
    }

    return fail(error_info{
      "Expected a numeric value, or a variable that refers to a numeric value.",
      idx
    });
  }

#define ARITHMETIC_OPERATOR(name, operation) \
  result_type name(unit operand) noexcept { \
    auto idx = operand.pos; \
    auto const& args = ::std::get<ls>(operand.expr).children; \
    if (args.size() < 2) { \
      return fail(error_info{ \
        "Expecting at least one argument.", \
        idx \
      }); \
    } \
    numeric result = 0.0; \
    for (::std::size_t idx = 1; idx < args.size(); ++idx) { \
      auto noe = numeric_or_error(args[idx]); \
      if (!noe) { \
        return noe; \
      } \
      result operation##= noe.value(); \
    } \
    return succeed(unit{idx, result}); \
  }

  ARITHMETIC_OPERATOR(add, +);
  ARITHMETIC_OPERATOR(sub, -);
  ARITHMETIC_OPERATOR(mul, *);
  ARITHMETIC_OPERATOR(div, /);

  result_type eval_m(unit operand) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<ls>(operand.expr).children;

    if (args.size() != 2) {
      return fail(error_info{
        "Eval expects a single argument.",
        idx
      });
    }

    auto const& arg = args[1];

    if (!::std::holds_alternative<ls>(arg.expr)) {
      return fail(error_info{
        "Eval expects a list type expression.",
        arg.pos
      });
    }

    auto const& list = ::std::get<ls>(arg.expr);

    if (!list.q) {
      auto maybe_q = eval(arg);
      if (!maybe_q) {
        return maybe_q;
      }

      if (!::std::holds_alternative<ls>(maybe_q.value().expr)
          || !::std::get<ls>(maybe_q.value().expr).q) {
        return fail(error_info{
          "Given expression does not yield a Q expression.",
          arg.pos
        });
      }

      return eval(maybe_q.value(), true); 
    }

    return eval(arg, true);
  }

  /*
   *
   * EVAL FUNCTIONS
   *
   */

  resolve_symbol_result resolve_symbol(unit const& pu) noexcept {
    if (!::std::holds_alternative<symbol>(pu.expr)) {
      return fail(error_info{
        "Expected a symbol.",
        pu.pos
      });
    }

    auto const& s = ::std::get<symbol>(pu.expr);

    ::std::unordered_map<symbol, expression> static env{
      {"+", function{"Adds numbers.", add}},
      {"-", function{"Subtracts numbers.", sub}},
      {"*", function{"Multiplies numbers.", mul}},
      {"/", function{"Divides numbers.", div}},
      {"eval", function{"Evaluates a Q expression.", eval_m}},
    };

    if (env.count(s)) {
      return succeed(unit{pu.pos, resolved_symbol{env[s]}});
    }

    return fail(error_info{
      concat("Symbol ", s, " is undefined."),
      pu.pos 
    });
  }

  result_type eval(unit const& pu, bool const eval_q) noexcept {
    if (::std::holds_alternative<numeric>(pu.expr)) {
      return succeed(pu);
    }

    if (::std::holds_alternative<symbol>(pu.expr)) {
      if (auto rsr = resolve_symbol(pu); rsr) {
        return succeed(rsr.value());
      } else {
        return fail(rsr.error());
      }
    }

    if (::std::holds_alternative<ls>(pu.expr)) {
      auto const& list = ::std::get<ls>(pu.expr);

      if (!eval_q && list.q) {
        return fail(error_info{
          "Expected an S expression in this context.",
          pu.pos
        });
      }

      if (list.children.empty()) {
        return fail(error_info{
          "Expected a non-empty expression.",
          pu.pos
        });
      }

      // ??
      if (list.children.size() == 1) {
        return eval(list.children.front());
      }

      auto either_operator = eval(list.children.front());

      if (!either_operator) {
        return either_operator;
      }

      auto const& maybe_operator = either_operator.value();

      if (!::std::holds_alternative<function>(maybe_operator.expr)) {
        return fail(error_info{
          "Expected a builtin or user defined function.",
          maybe_operator.pos
        });
      }

      return ::std::get<function>(maybe_operator.expr).func(pu);
    }

    terminate_with("Missing a type check in eval.");
    __builtin_unreachable();
  }

}
