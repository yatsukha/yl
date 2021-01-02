#include "yl/either.hpp"
#include "yl/types.hpp"
#include <cstdlib>
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
    auto first = numeric_or_error(args[1]); \
    if (!first) { \
      return first; \
    } \
    numeric result = first.value(); \
    for (::std::size_t idx = 2; idx < args.size(); ++idx) { \
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

    auto evaluated = eval(args[1]);

    if (!evaluated) {
      return evaluated;
    }

    auto arg = evaluated.value();

    if (!::std::holds_alternative<ls>(arg.expr)) {
      return fail(error_info{
        "Eval expects a list type expression.",
        arg.pos
      });
    }

    auto const& list = ::std::get<ls>(arg.expr);

    if (!list.q) { 
      return fail(error_info{
        "Eval expects a Q expression.",
        arg.pos
      });
    }

    return eval(arg, true);
  }

  result_type list_m(unit operand) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<ls>(operand.expr).children;

    if (args.size() < 2) {
      return fail(error_info{
        "List expects at least one argument.",
        idx
      });
    }

    auto q_expr = ls{.q = true};
    q_expr.children.reserve(args.size() - 1);
    for (::std::size_t i = 1; i < args.size(); ++i) {
      auto nc = eval(args[i]);
      if (!nc) {
        return nc;
      }
      q_expr.children.push_back(nc.value());
    }

    return succeed(unit{operand.pos, q_expr});
  }

  result_type head_m(unit operand) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<ls>(operand.expr).children;

    if (args.size() != 2) {
      return fail(error_info{
        "Head expects one argument.",
        idx
      });
    }

    auto evaluated = eval(args[1]);

    if (!evaluated) {
      return evaluated;
    }

    auto arg = evaluated.value();

    if (!::std::holds_alternative<ls>(arg.expr)
        || !::std::get<ls>(arg.expr).q){
      return fail(error_info{
        "Head expects a Q expression as an argument.",
        arg.pos
      });
    }

    auto const& other = ::std::get<ls>(arg.expr);

    if (other.children.empty()) {
      return fail(error_info{
        "Head takes a non-empty Q expression.",
        arg.pos
      });
    }

    return succeed(unit{
      other.children.front().pos, 
      ls{true, {other.children.front()}}
    });
  }

  result_type tail_m(unit operand) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<ls>(operand.expr).children;

    if (args.size() != 2) {
      return fail(error_info{
        "Tail expects one argument.",
        idx
      });
    }

    auto evaluated = eval(args[1]);

    if (!evaluated) {
      return evaluated;
    }

    auto arg = evaluated.value();

    if (!::std::holds_alternative<ls>(arg.expr)
        || !::std::get<ls>(arg.expr).q){
      return fail(error_info{
        "Tail expects a Q expression as an argument.",
        arg.pos
      });
    }

    auto const& other = ::std::get<ls>(arg.expr);

    return succeed(unit{
      arg.pos, 
      ls{true, {
        other.children.begin() + 1, 
        other.children.empty() ? other.children.begin() + 1 : other.children.end()
      }}
    });
  }

  result_type join_m(unit operand) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<ls>(operand.expr).children;

    if (args.size() < 2) {
      return fail(error_info{
        "Join expects at least one Q expression as an argument.",
        idx
      });
    }

    ls ret{true};

    for (::std::size_t i = 1; i < args.size(); ++i) {
      auto evaluated = eval(args[i]);

      if (!evaluated) {
        return evaluated;
      }

      auto arg = evaluated.value();

      if (!::std::holds_alternative<ls>(arg.expr)
          || !::std::get<ls>(arg.expr).q){
        return fail(error_info{
          "Join expects a Q expression as an argument.",
          arg.pos
        });
      }

      auto const& other = ::std::get<ls>(arg.expr);
      for (auto&& child : other.children) {
        ret.children.push_back(child);
      }
    }

    return succeed(unit{idx, ret});
  }

  result_type cons_m(unit operand) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<ls>(operand.expr).children;

    if (args.size() != 3) {
      return fail(error_info{
        "Cons expects two arguments, any expression and a Q expression.",
        idx
      });
    }

    auto evaluated = eval(args[2]);

    if (!evaluated) {
      return evaluated;
    }

    auto arg = evaluated.value();

    if (!::std::holds_alternative<ls>(arg.expr)
        || !::std::get<ls>(arg.expr).q){
      return fail(error_info{
        "Cons' second argument must be a Q expression.",
        arg.pos
      });
    }

    auto const& other = ::std::get<ls>(arg.expr);

    ls ret{true, {args[1]}};
    ret.children.insert(
        ret.children.end(), other.children.begin(), other.children.end());

    return succeed(unit{idx, ret});
  }

  result_type len_m(unit operand) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<ls>(operand.expr).children;

    if (args.size() != 2) {
      return fail(error_info{
        "Len expects a Q expression.",
        idx
      });
    }

    auto evaluated = eval(args[1]);

    if (!evaluated) {
      return evaluated;
    }

    auto arg = evaluated.value();

    if (!::std::holds_alternative<ls>(arg.expr)
        || !::std::get<ls>(arg.expr).q){
      return fail(error_info{
        "Len expects a Q expression.",
        arg.pos
      });
    }

    auto const& other = ::std::get<ls>(arg.expr);

    return succeed(unit{idx, numeric(other.children.size())});
  }

  result_type init_m(unit operand) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<ls>(operand.expr).children;

    if (args.size() != 2) {
      return fail(error_info{
        "Init expects one argument.",
        idx
      });
    }

    auto evaluated = eval(args[1]);

    if (!evaluated) {
      return evaluated;
    }

    auto arg = evaluated.value();

    if (!::std::holds_alternative<ls>(arg.expr)
        || !::std::get<ls>(arg.expr).q){
      return fail(error_info{
        "Init expects a Q expression as an argument.",
        arg.pos
      });
    }

    auto const& other = ::std::get<ls>(arg.expr);

    return succeed(unit{
      arg.pos, 
      ls{true, {
        other.children.begin(), 
        other.children.empty() ? other.children.begin() : other.children.end() - 1
      }}
    });
  }

  result_type def_m(unit u) noexcept;

  result_type env_m(unit u) noexcept;

  ::std::unordered_map<symbol, expression> static env{
    {"+", function{"Adds numbers.", add}},
    {"-", function{"Subtracts numbers.", sub}},
    {"*", function{"Multiplies numbers.", mul}},
    {"/", function{"Divides numbers.", div}},
    {"eval", function{"Evaluates a Q expression.", eval_m}},
    {"list", function{"Takes arguments and turns them into a Q expression.", list_m}},
    {"head", function{"Takes a Q expression and returns the first subexpression.", head_m}},
    {"tail", function{"Takes a Q expression and returns it without its 1st element.", tail_m}},
    {"join", function{"Joins one or more Q expressions.", join_m}},
    {"cons", function{"Appends its first argument to the second Q expression.", cons_m}},
    {"len", function{"Calculates the length of a Q expression.", len_m}},
    {"init", function{"Returns a Q expression without it's last element.", init_m}},
    {"def", function{
      "Assigns to symbols in a Q expression. 'def {a b} 1 2' assigns 1 and 2 to a and b.",
      def_m
    }},
  };

  // ?TODO: symbol or eval in arg list
 
  result_type def_m(unit u) noexcept {
    auto const& list = ::std::get<ls>(u.expr);
    
    if (list.children.size() < 3) {
      return fail(error_info{
        "Def expects at least 2 arguments.,\n"
        " - Q expression with variable names, or something that evaluates to Q expression,\n"
        " - values/expressions/etc. to assign",
        u.pos
      });
    }

    auto e_argl = eval(list.children[1]);

    if (!e_argl) {
      return e_argl;
    }

    auto const& [idx, args] = e_argl.value();

    if (!::std::holds_alternative<ls>(args)
        || !::std::get<ls>(args).q) {
      return fail(error_info{
        "Expected a Q expression for the argument list.",
        idx
      });
    }

    auto const& arguments = ::std::get<ls>(args);

    if (arguments.children.size() != list.children.size() - 2) {
      return fail(error_info{
        "Differing length of arguments and corresponding assignments.",
        idx
      });
    }

    for (::std::size_t i = 0; i < arguments.children.size(); ++i) {
      if (!::std::holds_alternative<symbol>(arguments.children[i].expr)) {
        return fail(error_info{
          "Unexpected non-symbol in the argument list.",
          arguments.children[i].pos
        });
      }

      auto assignment = eval(list.children[2 + i]);
      if (!assignment) {
        return assignment;
      }

      env[::std::get<symbol>(arguments.children[i].expr)] = assignment.value().expr;
    }

    return succeed(unit{u.pos, ls{}});
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

    auto s = ::std::get<symbol>(pu.expr);

    for (;;) {
      if (!env.count(s)) {
        return fail(error_info{
          concat("Symbol ", s, " is undefined."),
          pu.pos 
        });
      }

      auto next = env[s];

      if (!::std::holds_alternative<symbol>(next)) {
        return succeed(unit{pu.pos, next});
      }

      s = ::std::get<symbol>(next);
    }
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
        return succeed(pu);
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
