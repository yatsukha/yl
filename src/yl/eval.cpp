#include "yl/either.hpp"
#include "yl/types.hpp"
#include <cstdlib>
#include <debug/assertions.h>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <cmath>

#include <variant>
#include <yl/eval.hpp>

namespace yl {

#define FAIL_WITH(msg, pos) \
  return fail(error_info{ \
    msg, \
    pos \
  });

#define RETURN_IF_ERROR(either) \
  if (!either) { \
    return fail(either.error()); \
  }

#define ASSERT_ARG_COUNT(args, pos, eq) \
  if (!(args.size() - 1 eq)) { \
    FAIL_WITH("Argument count must be " #eq ".", pos) \
  }

#define DEF_TYPE_CHECK(type) \
  bool is_##type(expression const& expr) noexcept { \
    return ::std::holds_alternative<type>(expr); \
  }

  DEF_TYPE_CHECK(numeric);
  DEF_TYPE_CHECK(symbol);
  DEF_TYPE_CHECK(function);
  DEF_TYPE_CHECK(list);

#define Q_OR_ERROR(unit) \
  if (!is_list(unit.expr) \
      || !::std::get<list>(unit.expr).q) {\
    return fail(error_info{ \
      "Expected a Q expression.", \
       unit.pos \
    });  \
  }

  /*
   *
   * BUILTIN OPERATIONS
   *
   */

  either<error_info, numeric> numeric_or_error(unit u, env_node_ptr node) noexcept {
    if (is_numeric(u.expr)) {
      return succeed(::std::get<numeric>(u.expr));
    }

    FAIL_WITH("Expected a numeric value.", u.pos);
  }

#define ARITHMETIC_OPERATOR(name, operation) \
  result_type name(unit operand, env_node_ptr node) noexcept { \
    auto idx = operand.pos; \
    auto const& args = ::std::get<list>(operand.expr).children; \
    ASSERT_ARG_COUNT(args, idx, >= 1); \
    auto first = numeric_or_error(args[1], node); \
    RETURN_IF_ERROR(first); \
    numeric result = first.value(); \
    for (::std::size_t idx = 2; idx < args.size(); ++idx) { \
      auto noe = numeric_or_error(args[idx], node); \
      RETURN_IF_ERROR(noe); \
      result operation##= noe.value(); \
    } \
    return succeed(unit{idx, result}); \
  }

  ARITHMETIC_OPERATOR(add, +);
  ARITHMETIC_OPERATOR(sub, -);
  ARITHMETIC_OPERATOR(mul, *);
  ARITHMETIC_OPERATOR(div, /);

  result_type eval_m(unit operand, env_node_ptr node) noexcept {
    auto const& args = ::std::get<list>(operand.expr).children;

    ASSERT_ARG_COUNT(args, operand.pos, == 1);
    Q_OR_ERROR(args[1]);

    return eval(args[1], node, true);
  }

  result_type list_m(unit operand, env_node_ptr node) noexcept {
    auto const& args = ::std::get<list>(operand.expr).children;

    ASSERT_ARG_COUNT(args, operand.pos, >= 1);

    return succeed(unit{
      operand.pos, 
      list{
        .q = true, 
        .children = {args.begin() + 1, args.end()}
      }
    });
  }

  result_type head_m(unit operand, env_node_ptr node) noexcept {
    auto const& args = ::std::get<list>(operand.expr).children;

    ASSERT_ARG_COUNT(args, operand.pos, == 1);

    if (!is_list(args[1].expr) || !::std::get<list>(args[1].expr).q){
      FAIL_WITH("Head expects a Q expression as an argument.", args[1].pos);
    }

    auto const& ls = ::std::get<list>(args[1].expr);

    if (ls.children.empty()) {
      FAIL_WITH("Head takes a non-empty Q expression.", args[1].pos);
    }

    return succeed(unit{
      ls.children.front().pos, 
      list{true, {ls.children.front()}}
    });
  }

  result_type tail_m(unit operand, env_node_ptr node) noexcept {
    auto const& args = ::std::get<list>(operand.expr).children;

    ASSERT_ARG_COUNT(args, operand.pos, == 1);
    Q_OR_ERROR(args[1]);

    auto const& other = ::std::get<list>(args[1].expr);

    return succeed(unit{
      args[1].pos, 
      list{true, {
        other.children.begin() + 1, 
        other.children.empty() ? other.children.begin() + 1 : other.children.end()
      }}
    });
  }

  result_type join_m(unit operand, env_node_ptr node) noexcept {
    auto const& args = ::std::get<list>(operand.expr).children;

    ASSERT_ARG_COUNT(args, operand.pos, >= 1);

    list ret{true};

    for (::std::size_t i = 1; i < args.size(); ++i) {
      Q_OR_ERROR(args[i]);

      auto const& other = ::std::get<list>(args[i].expr);
      ret.children.insert(
        ret.children.end(), other.children.begin(), other.children.end());
    }

    return succeed(unit{operand.pos, ret});
  }

  result_type cons_m(unit operand, env_node_ptr node) noexcept {
    auto const& args = ::std::get<list>(operand.expr).children;

    ASSERT_ARG_COUNT(args, operand.pos, == 2);

    Q_OR_ERROR(args[2]);

    auto other = ::std::get<list>(args[2].expr);
    other.children.insert(other.children.begin(), args[1]);

    return succeed(unit{operand.pos, other});
  }

  result_type len_m(unit operand, env_node_ptr node) noexcept {
    auto const& args = ::std::get<list>(operand.expr).children;

    ASSERT_ARG_COUNT(args, operand.pos, == 1);
    Q_OR_ERROR(args[1]);

    return succeed(unit{
      operand.pos, 
      numeric(::std::get<list>(args[1].expr).children.size())
    });
  }

  result_type init_m(unit operand, env_node_ptr node) noexcept {
    auto const& args = ::std::get<list>(operand.expr).children;

    ASSERT_ARG_COUNT(args, operand.pos, == 1);
    Q_OR_ERROR(args[1]);

    auto const& other = ::std::get<list>(args[1].expr);

    return succeed(unit{
      args[1].pos, 
      list{true, {
        other.children.begin(), 
        other.children.empty() ? other.children.begin() : other.children.end() - 1
      }}
    });
  }
 
  result_type def_m(unit u, env_node_ptr node) noexcept {
    auto const& args = ::std::get<list>(u.expr).children;
  
    ASSERT_ARG_COUNT(args, u.pos, >= 2);
    Q_OR_ERROR(args[1]);

    auto const& arguments = ::std::get<list>(args[1].expr).children;

    auto g_env = global_environment();

    if (arguments.size() != args.size() - 2) {
      FAIL_WITH(
        "Differing length of arguments and corresponding assignments.",
        u.pos
      );
    }

    for (::std::size_t i = 0; i < arguments.size(); ++i) {
      if (!is_symbol(arguments[i].expr)) {
        FAIL_WITH("Unexpected non-symbol in the argument list.", arguments[i].pos);
      }

      (*g_env->curr)[::std::get<symbol>(arguments[i].expr)] = args[2 + i].expr;
    }

    return succeed(unit{u.pos, list{}});
  }

  function::type create_function(
    bool const variadic, bool const unused,
    list const arglist, list const body,
    position const body_pos,
    env_ptr self_env = {}
  ) noexcept {
    return [=](unit u, env_node_ptr private_env) mutable -> result_type {
      auto const& arguments = ::std::get<list>(u.expr).children;
      if (!variadic && arglist.children.size() < arguments.size() - 1) {
        FAIL_WITH(
          concat(
            "Excess arguments, expected ",
            arglist.children.size(),
            ", got ",
            arguments.size() - 1,
            "."
          ),
          u.pos
        );
      }

      bool partial = !variadic && arglist.children.size() > arguments.size() - 1;

      // TODO: does it work without this?
      // also evaluate the line above
      if (variadic 
          && arglist.children.size() > arguments.size() + !unused - 1) {
        FAIL_WITH(
          "Not enough values to assign to non-variadic parameters.",
          u.pos
        );
      }

      self_env = self_env ?: ::std::make_shared<environment>();

      for (::std::size_t i = 0; 
           i != (arglist.children.size() ? arguments.size() - 1 : 0); ++i) {
        auto sym = ::std::get<symbol>(arglist.children[i].expr);
        if (sym[0] == '&') {
          if (unused) {
            break;
          }
        
          (*self_env)[::std::get<symbol>(arglist.children[i + 1].expr)] = list{
            true, {arguments.begin() + i + 1, arguments.end()}
          };
          break;
        }

        (*self_env)[::std::get<symbol>(arglist.children[i].expr)] = 
          arguments[i + 1].expr;
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
                  arglist.children.begin() + arguments.size() - 1,
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

      return eval(
        unit{body_pos, body}, 
        ::std::make_shared<env_node>(env_node{
          .curr = self_env,
          .prev = private_env
        }), 
        true
      );
    };
  }

  result_type lambda_m(unit u, env_node_ptr node) noexcept {
    auto const& args = ::std::get<list>(u.expr).children;

    ASSERT_ARG_COUNT(args, u.pos, == 2);

    Q_OR_ERROR(args[1]);
    Q_OR_ERROR(args[2]);

    auto const arglist = ::std::get<list>(args[1].expr);

    bool variadic = arglist.children.size() == 0;
    bool unused   = variadic;

    for (::std::size_t i = 0; i < arglist.children.size(); ++i) {
      if (!::std::holds_alternative<symbol>(arglist.children[i].expr)) {
        FAIL_WITH("Expected a symbol.", arglist.children[i].pos);   
      }

      if (::std::get<symbol>(arglist.children[i].expr)[0] == '&') {
        if (variadic) {
          FAIL_WITH(
            "Can not have more than one variadic sign.", arglist.children[i].pos
          );
        }
        variadic = true;
        if (arglist.children.size() - i > 2) {
          FAIL_WITH(
            "Variadic sign expects either zero or one argument.",
            arglist.children[i].pos
          );
        }
        unused = i == arglist.children.size() - 1;
      }
    }

    return succeed(unit{
      u.pos,
      function{
        .description = "User defined function.",
        .func = create_function(
            variadic, unused, arglist, ::std::get<list>(args[2].expr), args[2].pos)
      }
    });
  }

  // .. abuse :^)
  result_type help_m(unit u, env_node_ptr env) noexcept {
    auto const& args = ::std::get<list>(u.expr).children;
    
    ASSERT_ARG_COUNT(args, u.pos, <= 1);

    symbol s{"\n"};

    if (args.size() == 1) {
      s +=
        "  Hi, welcome to yatsukha's lisp. This is a lisp like intepreted language.\n"
        "  There are only 4 types: numeric, symbol, function and list.\n"
        "  Numeric type is a floating point number such as 1, or 1.25.\n"
        "  Symbol is any named value such as 'a' or 'help'.\n"
        "  List is list of types. It can be evaluated such as '(+ 1 2)'\n"
        "  or unevaluated such as '{+ 1 2}' which can be evaluated using 'eval'.\n"
        "  Function is a resolved symbol that represents a computation,\n"
        "  it can be created using '\\', see 'help \\'.\n"
        "  Functions support partial evaluation.\n"
        "\n"
        "  Examples: \n"
        "  (+ 1 2)\n"
        "  eval {+ 1 2}\n"
        "  def {mySymbol} 2\n"
        "  + mySmbol 4\n"
        "  (\\{x y} {+ x y}) 2 4\n"
        "\n"
        "  Enter 'help symbol' to get information about a symbol.\n"
        "  Symbols currently available for inspection:\n";
      for (auto&& sym : *env->curr) {
        s += "    ";
        s += sym.first;
        s += "\n";
      }

      return succeed(unit{u.pos, list{true, {unit{u.pos, s}}}});
    }

    expression expr;

    if (is_symbol(args[1].expr)) {
      auto resolved = resolve_symbol({args[1].pos, args[1].expr}, env); 
      RETURN_IF_ERROR(resolved);

      expr = (*env->curr)[::std::get<symbol>(resolved.value().expr)];
    } else {
      expr = args[1].expr;
    }

    s += "  type: ";
    s += type_of(expr);
    s += "\n";

    s += "  value: ";

    ::std::stringstream ss;
    ss << expr << "\n";
    s += ss.str();
    
    return succeed(unit{u.pos, list{true, {unit{u.pos, s}}}});
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
        "Global assignment to symbols in a Q expression. "
        "For example 'def {a b} 1 2' assigns 1 and 2 to a and b.",
        def_m
      }},
      {"\\", function{
        "Lambda function, takes a Q expression argument list, "
        "and a Q expression body. "
        "For example '(\\{x y} {+ x y}) 2 3' will yield 5",
        lambda_m
      }},
      {"help", function{"Outputs information about a symbol.", help_m}},
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
    if (!is_symbol(pu.expr)) {
      FAIL_WITH("Expected a symbol.", pu.pos);
    }

    auto starting_point = node;
    auto s = ::std::get<symbol>(pu.expr);

    for (;;) {
      while (!(node->curr->count(s))) {
        if (!node->prev) {
          FAIL_WITH(concat("Symbol ", s, " is undefined."), pu.pos); 
        }
        node = node->prev;
      }

      auto next = node->curr->operator[](s);

      if (!is_symbol(next)) {
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
    if (is_numeric(pu.expr)) {
      return succeed(pu);
    }

    if (is_symbol(pu.expr)) {
      if (auto rsr = resolve_symbol(pu, node); rsr) {
        return succeed(rsr.value());
      } else {
        return fail(rsr.error());
      }
    }

    if (is_list(pu.expr)) {
      auto ls = ::std::get<list>(pu.expr);

      if (!eval_q && ls.q) {
        return succeed(pu);
      }

      if (ls.children.empty()) {
        FAIL_WITH("Expected a non-empty expression.", pu.pos);
      }
      
      for (auto& child : ls.children) {
        auto new_child = eval(child, node);
        RETURN_IF_ERROR(new_child);
        child = new_child.value();
      }

      if (ls.children.size() == 1 && !is_function(ls.children.front().expr)) {
        return succeed(ls.children.front());
      }

      if (!is_function(ls.children.front().expr)) {
        FAIL_WITH(
          "Expected a builtin or user defined function.",
          ls.children.front().pos
        );
      }

      auto const& fn = ::std::get<function>(ls.children.front().expr);
      return fn.func({pu.pos, ls}, node);
    }

    if (is_function(pu.expr)) {
      return succeed(pu);
    }

    terminate_with("Missing a type check in eval.");
    __builtin_unreachable();
  }

}
