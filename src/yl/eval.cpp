#include "yl/either.hpp"
#include "yl/types.hpp"
#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <cmath>

#include <variant>
#include <yl/eval.hpp>

namespace yl {

  /*
   *
   * BUILTIN OPERATIONS
   *
   */

  either<error_info, numeric> numeric_or_error(unit u, env_node_ptr node) noexcept {
    auto ret = eval(u, node);

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
  result_type name(unit operand, env_node_ptr node) noexcept { \
    auto idx = operand.pos; \
    auto const& args = ::std::get<list>(operand.expr).children; \
    if (args.size() < 2) { \
      return fail(error_info{ \
        "Expecting at least one argument.", \
        idx \
      }); \
    } \
    auto first = numeric_or_error(args[1], node); \
    if (!first) { \
      return first; \
    } \
    numeric result = first.value(); \
    for (::std::size_t idx = 2; idx < args.size(); ++idx) { \
      auto noe = numeric_or_error(args[idx], node); \
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

  result_type eval_m(unit operand, env_node_ptr node) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<list>(operand.expr).children;

    if (args.size() != 2) {
      return fail(error_info{
        "Eval expects a single argument.",
        idx
      });
    }

    auto evaluated = eval(args[1], node);

    if (!evaluated) {
      return evaluated;
    }

    auto arg = evaluated.value();

    if (!::std::holds_alternative<list>(arg.expr)) {
      return fail(error_info{
        "Eval expects a list type expression.",
        arg.pos
      });
    }

    auto const& ls = ::std::get<list>(arg.expr);

    if (!ls.q) { 
      return fail(error_info{
        "Eval expects a Q expression.",
        arg.pos
      });
    }

    return eval(arg, node, true);
  }

  result_type list_m(unit operand, env_node_ptr node) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<list>(operand.expr).children;

    if (args.size() < 2) {
      return fail(error_info{
        "List expects at least one argument.",
        idx
      });
    }

    auto q_expr = list{.q = true};
    q_expr.children.reserve(args.size() - 1);
    for (::std::size_t i = 1; i < args.size(); ++i) {
      auto nc = eval(args[i], node);
      if (!nc) {
        return nc;
      }
      q_expr.children.push_back(nc.value());
    }

    return succeed(unit{operand.pos, q_expr});
  }

  result_type head_m(unit operand, env_node_ptr node) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<list>(operand.expr).children;

    if (args.size() != 2) {
      return fail(error_info{
        "Head expects one argument.",
        idx
      });
    }

    auto evaluated = eval(args[1], node);

    if (!evaluated) {
      return evaluated;
    }

    auto arg = evaluated.value();

    if (!::std::holds_alternative<list>(arg.expr)
        || !::std::get<list>(arg.expr).q){
      return fail(error_info{
        "Head expects a Q expression as an argument.",
        arg.pos
      });
    }

    auto const& other = ::std::get<list>(arg.expr);

    if (other.children.empty()) {
      return fail(error_info{
        "Head takes a non-empty Q expression.",
        arg.pos
      });
    }

    return succeed(unit{
      other.children.front().pos, 
      list{true, {other.children.front()}}
    });
  }

  result_type tail_m(unit operand, env_node_ptr node) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<list>(operand.expr).children;

    if (args.size() != 2) {
      return fail(error_info{
        "Tail expects one argument.",
        idx
      });
    }

    auto evaluated = eval(args[1], node);

    if (!evaluated) {
      return evaluated;
    }

    auto arg = evaluated.value();

    if (!::std::holds_alternative<list>(arg.expr)
        || !::std::get<list>(arg.expr).q){
      return fail(error_info{
        "Tail expects a Q expression as an argument.",
        arg.pos
      });
    }

    auto const& other = ::std::get<list>(arg.expr);

    return succeed(unit{
      arg.pos, 
      list{true, {
        other.children.begin() + 1, 
        other.children.empty() ? other.children.begin() + 1 : other.children.end()
      }}
    });
  }

  result_type join_m(unit operand, env_node_ptr node) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<list>(operand.expr).children;

    if (args.size() < 2) {
      return fail(error_info{
        "Join expects at least one Q expression as an argument.",
        idx
      });
    }

    list ret{true};

    for (::std::size_t i = 1; i < args.size(); ++i) {
      auto evaluated = eval(args[i], node);

      if (!evaluated) {
        return evaluated;
      }

      auto arg = evaluated.value();

      if (!::std::holds_alternative<list>(arg.expr)
          || !::std::get<list>(arg.expr).q){
        return fail(error_info{
          "Join expects a Q expression as an argument.",
          arg.pos
        });
      }

      auto const& other = ::std::get<list>(arg.expr);
      for (auto&& child : other.children) {
        ret.children.push_back(child);
      }
    }

    return succeed(unit{idx, ret});
  }

  result_type cons_m(unit operand, env_node_ptr node) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<list>(operand.expr).children;

    if (args.size() != 3) {
      return fail(error_info{
        "Cons expects two arguments, any expression and a Q expression.",
        idx
      });
    }

    auto evaluated = eval(args[2], node);

    if (!evaluated) {
      return evaluated;
    }

    auto arg = evaluated.value();

    if (!::std::holds_alternative<list>(arg.expr)
        || !::std::get<list>(arg.expr).q){
      return fail(error_info{
        "Cons' second argument must be a Q expression.",
        arg.pos
      });
    }

    auto const& other = ::std::get<list>(arg.expr);

    list ret{true, {args[1]}};
    ret.children.insert(
        ret.children.end(), other.children.begin(), other.children.end());

    return succeed(unit{idx, ret});
  }

  result_type len_m(unit operand, env_node_ptr node) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<list>(operand.expr).children;

    if (args.size() != 2) {
      return fail(error_info{
        "Len expects a Q expression.",
        idx
      });
    }

    auto evaluated = eval(args[1], node);

    if (!evaluated) {
      return evaluated;
    }

    auto arg = evaluated.value();

    if (!::std::holds_alternative<list>(arg.expr)
        || !::std::get<list>(arg.expr).q){
      return fail(error_info{
        "Len expects a Q expression.",
        arg.pos
      });
    }

    auto const& other = ::std::get<list>(arg.expr);

    return succeed(unit{idx, numeric(other.children.size())});
  }

  result_type init_m(unit operand, env_node_ptr node) noexcept {
    auto idx = operand.pos;
    auto const& args = ::std::get<list>(operand.expr).children;

    if (args.size() != 2) {
      return fail(error_info{
        "Init expects one argument.",
        idx
      });
    }

    auto evaluated = eval(args[1], node);

    if (!evaluated) {
      return evaluated;
    }

    auto arg = evaluated.value();

    if (!::std::holds_alternative<list>(arg.expr)
        || !::std::get<list>(arg.expr).q){
      return fail(error_info{
        "Init expects a Q expression as an argument.",
        arg.pos
      });
    }

    auto const& other = ::std::get<list>(arg.expr);

    return succeed(unit{
      arg.pos, 
      list{true, {
        other.children.begin(), 
        other.children.empty() ? other.children.begin() : other.children.end() - 1
      }}
    });
  }

  // ?TODO: symbol or eval in arg list
 
  result_type def_m(unit u, env_node_ptr node) noexcept {
    auto const& ls = ::std::get<list>(u.expr);
    
    if (ls.children.size() < 3) {
      return fail(error_info{
        "Def expects at least 2 arguments.,\n"
        " - Q expression with variable names, or something that evaluates to Q expression,\n"
        " - values/expressions/etc. to assign",
        u.pos
      });
    }

    auto e_argl = eval(ls.children[1], node);

    if (!e_argl) {
      return e_argl;
    }

    auto const& [idx, args] = e_argl.value();

    if (!::std::holds_alternative<list>(args)
        || !::std::get<list>(args).q) {
      return fail(error_info{
        "Expected a Q expression for the argument list.",
        idx
      });
    }

    auto const& arguments = ::std::get<list>(args);

    auto g_env = node;

    while (g_env->prev) {
      g_env = g_env->prev;
    }

    if (arguments.children.size() != ls.children.size() - 2) {
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

      auto assignment = eval(ls.children[2 + i], node);
      if (!assignment) {
        return assignment;
      }

      (*g_env->curr)[::std::get<symbol>(arguments.children[i].expr)] = 
        assignment.value().expr;
    }

    return succeed(unit{u.pos, list{}});
  }

// TODO: move further up and refactor
#define Q_OR_ERROR(expr, idx) \
  if (!::std::holds_alternative<list>(expr) \
      || !::std::get<list>(expr).q) {\
    return fail(error_info{ \
      "Expected a Q expression.", \
       idx \
    });  \
  }

  function::type create_function(
    bool const variadic, bool const unused,
    list const arglist, list const body,
    position const body_pos,
    env_ptr self_env = {}
  ) noexcept {
    return [=](unit u, env_node_ptr private_env) mutable -> result_type {
      auto const& ls = ::std::get<list>(u.expr);
      if (!variadic && arglist.children.size() < ls.children.size() - 1) {
        return fail(error_info{
          concat(
            "Excess arguments, expected ",
            arglist.children.size(),
            ", got ",
            ls.children.size() - 1,
            "."
          ),
          u.pos
        });
      }

      bool partial = !variadic && arglist.children.size() > ls.children.size() - 1;

      if (variadic 
          && arglist.children.size() > ls.children.size() + 1 + !unused) {
        return fail(error_info{
          "Not enough values to assign to non-variadic parameters.",
          u.pos
        });
      }

      if (!self_env) {
        self_env = ::std::make_shared<environment>();
      }

      for (::std::size_t i = 0; i < ls.children.size() - 1; ++i) {
        auto sym = ::std::get<symbol>(arglist.children[i].expr);
        if (sym[0] == '&') {
          if (unused) {
            break;
          }

          list q{.q = true};

          for (::std::size_t j = i + 1; j < ls.children.size(); ++j) {     
            auto evaluated = eval(ls.children[j], private_env);
            if (!evaluated) {
              return evaluated;
            }
            q.children.push_back(evaluated.value());
          }
        
          (*self_env)[::std::get<symbol>(arglist.children[i + 1].expr)] = q;
          break;
        }

        auto evaluated = eval(ls.children[i + 1], private_env);
        if (!evaluated) {
          return evaluated;
        }
        (*self_env)[::std::get<symbol>(arglist.children[i].expr)] = 
          evaluated.value().expr;
      }

      if (partial) {
        return succeed(unit{body_pos, function{
          .description = "User defined partially evaluated function.",
          .func = [=](unit nested_u, env_node_ptr nested_env) -> result_type {
            return create_function(
              false, false, 
              {
                .q = true,
                .children = {
                  arglist.children.begin() + ls.children.size() - 1,
                  arglist.children.end()
                }
              },
              body,
              body_pos,
              self_env
            )(nested_u, nested_env);
          }
        }});
      }

      env_node new_env{
        .curr = self_env,
        .prev = private_env
      };

      return eval(unit{body_pos, body}, ::std::make_shared<env_node>(new_env), true);
    };
  }

  result_type lambda_m(unit u, env_node_ptr node) noexcept {
    auto const& ls = ::std::get<list>(u.expr);

    if (ls.children.size() != 3) {
      return fail(error_info{
        "Lambda expects two arguments.",
        u.pos
      });
    }

    auto agg = aggregate(eval(ls.children[1], node), eval(ls.children[2], node));

    if (!agg) {
      return agg;
    }

    auto const& [v_arglist, v_body] = agg.value();

    Q_OR_ERROR(v_arglist.expr, v_arglist.pos);
    Q_OR_ERROR(v_body.expr, v_body.pos);

    auto const arglist = ::std::get<list>(v_arglist.expr);
    auto const body    = ::std::get<list>(v_body.expr);

    bool variadic = arglist.children.size() == 0;
    bool unused   = variadic;

    for (::std::size_t i = 0; i < arglist.children.size(); ++i) {
      if (!::std::holds_alternative<symbol>(arglist.children[i].expr)) {
        return fail(error_info{
          "Expected a symbol.",
          arglist.children[i].pos
        });   
      }

      if (::std::get<symbol>(arglist.children[i].expr)[0] == '&') {
        if (variadic) {
          return fail(error_info{
            "Can not have more than one variadic sign.",
            arglist.children[i].pos
          });
        }
        variadic = true;
        if (arglist.children.size() - i > 2) {
          return fail(error_info{
            "Variadic sign expects either zero or one argument.",
            arglist.children[i].pos
          });
        }
        unused = i == arglist.children.size() - 1;
      }
    }

    return succeed(unit{
      u.pos,
      function{
        .description = "User defined function.",
        .func = create_function(variadic, unused, arglist, body, v_body.pos)
      }
    });
  }
   
  env_node_ptr global_environment() noexcept {
    auto static g_env = ::std::make_shared<environment>(environment{
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
        "Global assignment to symbols in a Q expression.\n"
        "'def {a b} 1 2' assigns 1 and 2 to a and b.",
        def_m
      }},
      {"\\", function{
        "Lambda function, takes a Q expression argument list, "
        "and a Q expression body.\n"
        "Example: (\\{x y} {+ x y}) 2 3\n"
        "Result: 5",
        lambda_m
      }},
    });

    return ::std::make_shared<env_node>(env_node{
      .curr = g_env,
      .prev = {}
    });
  }



  /*
   *
   * EVAL FUNCTIONS
   *
   */

  result_type resolve_symbol(unit const& pu, env_node_ptr node) noexcept {
    if (!::std::holds_alternative<symbol>(pu.expr)) {
      return fail(error_info{
        "Expected a symbol.",
        pu.pos
      });
    }

    auto starting_point = node;
    auto s = ::std::get<symbol>(pu.expr);

    for (;;) {
      while (!(node->curr->count(s))) {
        if (!node->prev) {
          return fail(error_info{
            concat("Symbol ", s, " is undefined."),
            pu.pos 
          });     
        }

        node = node->prev;
      }

      auto next = node->curr->operator[](s);

      if (!::std::holds_alternative<symbol>(next)) {
        return succeed(unit{pu.pos, next});
      }

      s = ::std::get<symbol>(next);
      node = starting_point;
    }
  }

  result_type eval(
    unit const& pu, 
    env_node_ptr node, 
    bool const eval_q
  ) noexcept {
    if (::std::holds_alternative<numeric>(pu.expr)) {
      return succeed(pu);
    }

    if (::std::holds_alternative<symbol>(pu.expr)) {
      if (auto rsr = resolve_symbol(pu, node); rsr) {
        return succeed(rsr.value());
      } else {
        return fail(rsr.error());
      }
    }

    if (::std::holds_alternative<list>(pu.expr)) {
      auto const& ls = ::std::get<list>(pu.expr);

      if (!eval_q && ls.q) {
        return succeed(pu);
      }

      if (ls.children.empty()) {
        return fail(error_info{
          "Expected a non-empty expression.",
          pu.pos
        });
      }

      // ??
      /*if (ls.children.size() == 1) {
        return eval(ls.children.front(), node);
      }*/

      auto either_operator = eval(ls.children.front(), node);

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

      auto const& fn = ::std::get<function>(maybe_operator.expr);
      return fn.func(pu, node);
    }

    if (::std::holds_alternative<function>(pu.expr)) {
      return succeed(pu);
    }

    terminate_with("Missing a type check in eval.");
    __builtin_unreachable();
  }

}
