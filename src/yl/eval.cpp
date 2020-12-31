#include "yl/either.hpp"
#include "yl/parse.hpp"
#include "yl/util.hpp"
#include <bits/c++config.h>
#include <stdexcept>
#include <string>
#include <functional>
#include <unordered_map>
#include <cmath>
#include <yl/eval.hpp>

namespace yl {

  using operands = ::std::vector<poly_base>&;
  using operation = ::std::function<eval_either(operands)>;

  eval_either eval_s(poly_base&) noexcept;

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
      result_type::success_type running_total; \
      for (::std::size_t i = 1; i < args.size(); ++i) { \
        auto evaluated = eval_s(args[i]); \
        RETURN_IF_ERROR(evaluated); \
        RETURN_ERROR_IF_Q(evaluated); \
        auto const value = evaluated.value().value(); \
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
  }

}
