#include "yl/either.hpp"
#include "yl/types.hpp"
#include <cstdlib>
#include <debug/assertions.h>
#include <iostream>
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

  bool is_q(unit u) noexcept {
    return is_list(u.expr) && ::std::get<list>(u.expr).q;
  }

  bool is_raw(unit u) noexcept {
    return is_string(u.expr) && ::std::get<string>(u.expr).raw;
  }

#define Q_OR_ERROR(unit) \
  if (!is_q(unit)) {\
    return fail(error_info{ \
      "Expected a Q expression.", \
       unit.pos \
    });  \
  }

#define RAW_OR_ERROR(unit) \
  if (!is_raw(unit)) { \
    FAIL_WITH("Expected a raw string.", unit.pos); \
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
  result_type name##_m(unit operand, env_node_ptr node) noexcept { \
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
  ARITHMETIC_OPERATOR(mod, %);

  ARITHMETIC_OPERATOR(and, &);
  ARITHMETIC_OPERATOR(or, |);
  ARITHMETIC_OPERATOR(xor, ^);

  ARITHMETIC_OPERATOR(shl, <<);
  ARITHMETIC_OPERATOR(shr, >>);

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

  result_type head_m(unit u, env_node_ptr node) noexcept {
    auto const& args = ::std::get<list>(u.expr).children;

    ASSERT_ARG_COUNT(args, u.pos, == 1);

    bool q;

    if (!(q = is_q(args[1])) && !is_raw(args[1])) {
      FAIL_WITH("Head expects a Q expression or a raw string as an argument.", 
                args[1].pos);
    }

    if (q) {
      auto const& ls = as_list(args[1].expr);
      auto const empty = ls.children.empty();
      return succeed(unit{
        empty ? args[1].pos : ls.children.front().pos,
        list{
          true, 
          empty ? list::children_type{} : list::children_type{ls.children.front()}
        }
      });
    }

    auto const& str = as_string(args[1].expr).str;

    return succeed(unit{
      args[1].pos,
      string{
        .str = str.empty() 
          ? ::std::string{""} 
          : ::std::string{str.begin(), str.begin() + 1},
        .raw = true
      }
    });
  }

  result_type tail_m(unit u, env_node_ptr node) noexcept {
    auto const& args = ::std::get<list>(u.expr).children;

    ASSERT_ARG_COUNT(args, u.pos, == 1);

    bool q;

    if (!(q = is_q(args[1])) && !is_raw(args[1])) {
      FAIL_WITH("Tail expects a Q expression or a raw string as an argument.", 
                args[1].pos);
    }

    if (q) {
      auto const& ls = as_list(args[1].expr);
      return succeed(unit{
        args[1].pos,
        list{
          true, 
          {
            ls.children.begin() + 1, 
            ls.children.empty() 
              ? ls.children.begin() + 1
              : ls.children.end()
          } 
        }
      });
    }

    auto const& str = as_string(args[1].expr).str;

    return succeed(unit{
      args[1].pos,
      string{
      .str = {str.begin() + 1, str.empty() ? str.begin() + 1 : str.end()},
        .raw = true
      }
    });
  }

  result_type join_m(unit u, env_node_ptr node) noexcept {
    auto const& args = ::std::get<list>(u.expr).children;

    ASSERT_ARG_COUNT(args, u.pos, >= 1);

    bool q;

    if (!(q = is_q(args[1])) && !is_raw(args[1])) {
      FAIL_WITH("Tail expects a Q expression or a raw string as an argument.", 
                args[1].pos);
    }

    if (q) {
      auto ret = list{true, {}};
      for (::std::size_t i = 1; i < args.size(); ++i) {
        Q_OR_ERROR(args[i]);
        auto const& other = as_list(args[i].expr);
        ret.children.insert(ret.children.end(), 
                            other.children.begin(), other.children.end());
      }
      return succeed(unit{u.pos, ::std::move(ret)});
    }

    ::std::string str;
    for (::std::size_t i = 1; i < args.size(); ++i) {
      RAW_OR_ERROR(args[i]);
      str += as_string(args[i].expr).str;
    }

    return succeed(unit{u.pos, string{::std::move(str), true}});
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

    bool q;
    if (!(q = is_q(args[1])) && !is_raw(args[1])) {
      FAIL_WITH("Expected a Q expression or a raw string.", args[1].pos);
    }

    return succeed(unit{
      operand.pos, 
      numeric(q 
        ? as_list(args[1].expr).children.size() 
        : as_string(args[1].expr).str.size())
    });
  }

  result_type init_m(unit u, env_node_ptr node) noexcept {
    auto const& args = ::std::get<list>(u.expr).children;

    ASSERT_ARG_COUNT(args, u.pos, == 1);

    bool q;

    if (!(q = is_q(args[1])) && !is_raw(args[1])) {
      FAIL_WITH("Tail expects a Q expression or a raw string as an argument.", 
                args[1].pos);
    }

    if (q) {
      auto const& ls = as_list(args[1].expr);
      return succeed(unit{
        args[1].pos,
        list{
          true, 
          {
            ls.children.begin(), 
            ls.children.empty() 
              ? ls.children.begin()
              : ls.children.end() - 1
          } 
        }
      });
    }

    auto const& str = as_string(args[1].expr).str;

    return succeed(unit{
      args[1].pos,
      string{
      .str = {str.begin(), str.empty() ? str.begin() : str.end() - 1},
        .raw = true
      }
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
      if (!is_string(arguments[i].expr)
          || ::std::get<string>(arguments[i].expr).raw) {
        FAIL_WITH("Unexpected non-symbol in the argument list.", arguments[i].pos);
      }

      (*g_env->curr)[::std::get<string>(arguments[i].expr).str] = args[2 + i].expr;
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
        auto sym = ::std::get<string>(arglist.children[i].expr);
        if (sym.str[0] == '&') {
          if (unused) {
            break;
          }
        
          (*self_env)[::std::get<string>(arglist.children[i + 1].expr).str] = list{
            true, {arguments.begin() + i + 1, arguments.end()}
          };
          break;
        }

        (*self_env)[::std::get<string>(arglist.children[i].expr).str] = 
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

    ASSERT_ARG_COUNT(args, u.pos, >= 2);
    ASSERT_ARG_COUNT(args, u.pos, <= 3);

    Q_OR_ERROR(args[1]);

    ::std::string doc_string = "User defined function.";

    if (args.size() == 4) {
      Q_OR_ERROR(args[3]);
      if (!is_string(args[2].expr) || !::std::get<string>(args[2].expr).raw) {
        FAIL_WITH("Expected a raw doc-string.", args[2].pos);
      }
      doc_string = ::std::get<string>(args[2].expr).str;
    } else {
      Q_OR_ERROR(args[2]);
    }

    auto const arglist = ::std::get<list>(args[1].expr);

    bool variadic = arglist.children.size() == 0;
    bool unused   = variadic;

    for (::std::size_t i = 0; i < arglist.children.size(); ++i) {
      if (!is_string(arglist.children[i].expr) 
          || ::std::get<string>(arglist.children[i].expr).raw) {
        FAIL_WITH("Expected a symbol.", arglist.children[i].pos);   
      }

      if (::std::get<string>(arglist.children[i].expr).str[0] == '&') {
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

    auto const& body = args.size() == 4 ? args[3] : args[2];

    return succeed(unit{
      u.pos,
      function{
        .description = ::std::move(doc_string),
        .func = create_function(
            variadic, unused, arglist, ::std::get<list>(body.expr), body.pos)
      }
    });
  }

  result_type help_m(unit u, env_node_ptr env) noexcept {
    auto const& args = ::std::get<list>(u.expr).children;
    
    ASSERT_ARG_COUNT(args, u.pos, <= 1);

    string s{"\n", true};

    if (args.size() == 1) {
      s.str +=
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
        s.str += "    ";
        s.str += sym.first;
        s.str += "\n";
      }

      return succeed(unit{u.pos, list{
          true, {unit{u.pos, s}} 
      }});
    }

    expression expr;

    if (is_string(args[1].expr)) {
      auto resolved = resolve_symbol({args[1].pos, args[1].expr}, env); 
      RETURN_IF_ERROR(resolved);

      expr = (*env->curr)[::std::get<string>(resolved.value().expr).str];
    } else {
      expr = args[1].expr;
    }

    s.str += "  type: ";
    s.str += type_of(expr);
    s.str += "\n";

    s.str += "  value: ";

    ::std::stringstream ss;
    ss << expr << "\n";
    s.str += ss.str();
    
    return succeed(unit{u.pos, s});
  }

  /*
   *
   * COMPARISON AND ORDERING
   *
   */

  either<error_info, bool> operator==(unit const& a, unit const& b) noexcept {
    if (a.expr.index() != b.expr.index()) {
      return succeed(false);
    }

    if (is_numeric(a.expr)) {
      return succeed(::std::get<numeric>(a.expr) == ::std::get<numeric>(b.expr));
    }

    if (is_string(a.expr)) {
      auto const& sa = ::std::get<string>(a.expr);
      auto const& sb = ::std::get<string>(b.expr);
      return succeed(sa.raw == sb.raw && sa.str == sb.str);
    }

    if (is_list(a.expr)) {
      auto const& al = ::std::get<list>(a.expr);
      auto const& bl = ::std::get<list>(b.expr);

      return succeed(al.q == bl.q && al.children == bl.children);
    }
    
    FAIL_WITH("Function comparison is unsupported.", a.pos);
  }

  result_type equal_m(unit u, env_node_ptr env) noexcept {
    auto const& args = ::std::get<list>(u.expr).children;
    ASSERT_ARG_COUNT(args, u.pos, == 2);
    auto ret = args[1] == args[2];
    RETURN_IF_ERROR(ret); 
    return succeed(unit{u.pos, static_cast<numeric>(ret.value())});
  }


  result_type not_equal_m(unit u, env_node_ptr env) noexcept {
    auto ret = equal_m(u, env);
    RETURN_IF_ERROR(ret);
    return succeed(unit{u.pos, !::std::get<numeric>(ret.value().expr)});;
  }

#define SIMPLE_ORDERING(name, op) \
  result_type name##_m(unit u, env_node_ptr env) noexcept { \
    auto const& args = ::std::get<list>(u.expr).children; \
    ASSERT_ARG_COUNT(args, u.pos, == 2); \
    if (args[1].expr.index() != args[1].expr.index() \
        || !is_numeric(args[1].expr)) { \
      FAIL_WITH("Expected two numeric arguments", u.pos);  \
    } \
    return succeed(unit{u.pos, static_cast<numeric>( \
      ::std::get<numeric>(args[1].expr) op ::std::get<numeric>(args[2].expr) \
    )}); \
  }

  SIMPLE_ORDERING(less_than, <);
  SIMPLE_ORDERING(greater_than, >);
  SIMPLE_ORDERING(less_or_equal, <=);
  SIMPLE_ORDERING(greater_or_equal, >=);

  result_type if_m(unit u, env_node_ptr env) noexcept {
    auto const& args = ::std::get<list>(u.expr).children;

    ASSERT_ARG_COUNT(args, u.pos, >= 2);
    ASSERT_ARG_COUNT(args, u.pos, <= 3);

    bool has_else = args.size() == 4;

    if (!is_numeric(args[1].expr)) {
      FAIL_WITH("Expected a numeric value.", u.pos);
    }

    Q_OR_ERROR(args[2]);
    if (has_else) {
      Q_OR_ERROR(args[3]);
    }

    if (!::std::get<numeric>(args[1].expr)) {
      if (has_else) {
        return eval(args[3], env, true);
      }
      return eval(unit{u.pos, list{.q = false, .children = {}}});
    } else {
      return eval(args[2], env, true);
    }
  }
   
  env_node_ptr global_environment() noexcept {
    auto static g_env = ::std::make_shared<environment>(environment{
      {"+", function{"Adds numbers.", add_m}},
      {"-", function{"Subtracts numbers.", sub_m}},
      {"*", function{"Multiplies numbers.", mul_m}},
      {"/", function{"Divides numbers.", div_m}},
      {"%", function{"Modulo.", mod_m}},
      {"&", function{"Binary and.", and_m}},
      {"|", function{"Binary or.", or_m}},
      {"^", function{"Binary xor.", xor_m}},
      {"<<", function{"Shift left.", shl_m}},
      {">>", function{"Shift right.", shr_m}},
      {"eval", function{"Evaluates a Q expression.", eval_m}},
      {"list", function{"Takes arguments and turns them into a Q expression.", list_m}},
      {"head", function{
        "Takes a Q expression or a raw string and returns the first subexpression.", head_m
      }},
      {"tail", function{
        "Takes a Q expression or a raw string and returns it without its 1st element.", tail_m
      }},
      {"join", function{"Joins one or more Q expressions or raw strings.", join_m}},
      {"cons", function{"Appends its first argument to the second Q expression.", cons_m}},
      {"len", function{"Calculates the length of a Q expression or a raw string.", len_m}},
      {"init", function{"Returns a Q expression or a raw string without it's last element.", init_m}},
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
      {"==", function{
        "Compares arguments for equality. Can not compare functions.",
        equal_m
      }},
      {"!=", function{
        "Compares arguments for non equality. Can not compare functions.",
        not_equal_m
      }},
      {"<", function{"Tests if first number is less than second.", less_than_m}},
      {">", function{"Tests if first number is greater than second.", greater_than_m}},
      {"<=", function{"Tests if first number is less than or equal to second.", less_or_equal_m}},
      {">=", function{"Tests if first number is greater than or equal to second.", greater_or_equal_m}},
      {"if", function{
        "If(-else) statement. Takes a numeric for test. "
        "If it is != 0 it executes the first given Q expression. "
        "Otherwise, if it exists, it executes the second Q expression.",
        if_m
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
    if (!is_string(pu.expr)) {
      FAIL_WITH("Expected a symbol.", pu.pos);
    }

    auto starting_point = node;
    auto s = ::std::get<string>(pu.expr).str;

    for (;;) {
      while (!(node->curr->count(s))) {
        if (!node->prev) {
          FAIL_WITH(concat("Symbol ", s, " is undefined."), pu.pos); 
        }
        node = node->prev;
      }

      auto next = node->curr->operator[](s);

      if (!is_string(next) || ::std::get<string>(next).raw) {
        return succeed(unit{pu.pos, next});
      }

      s = ::std::get<string>(next).str;
      node = starting_point;
    }
  }

  result_type eval(
    unit const& pu, 
    env_node_ptr node, 
    bool const force_eval
  ) noexcept {
    if (is_numeric(pu.expr)) {
      return succeed(pu);
    }

    if (is_string(pu.expr)) {
      if (::std::get<string>(pu.expr).raw) {
        return succeed(pu);
      }

      if (auto rsr = resolve_symbol(pu, node); rsr) {
        return succeed(rsr.value());
      } else {
        return fail(rsr.error());
      }
    }

    if (is_list(pu.expr)) {
      auto ls = ::std::get<list>(pu.expr);

      if (!force_eval && ls.q) {
        return succeed(pu);
      }

      if (ls.children.empty()) {
        return succeed(pu);
        //FAIL_WITH("Expected a non-empty expression.", pu.pos);
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
