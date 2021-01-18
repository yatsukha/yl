#include "yl/either.hpp"
#include "yl/mem.hpp"
#include "yl/types.hpp"
#include <cstdlib>
#include <debug/assertions.h>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <cmath>

#include <variant>
#include <yl/eval.hpp>

namespace yl {

#define SUCCEED_WITH(pos, expr) \
  return succeed(make_shared<unit>(pos, expr));

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

  bool is_q(unit_ptr const& u) noexcept {
    return is_list(u->expr) && as_list(u->expr).q;
  }

  bool is_raw(unit_ptr const& u) noexcept {
    return is_string(u->expr) && as_string(u->expr).raw;
  }

#define Q_OR_ERROR(unit_ptr) \
  if (!is_q(unit_ptr)) {\
    return fail(error_info{ \
      "Expected a Q expression.", \
       unit_ptr->pos \
    });  \
  }

#define RAW_OR_ERROR(unit_ptr) \
  if (!is_raw(unit_ptr)) { \
    FAIL_WITH("Expected a raw string.", unit_ptr->pos); \
  }

  /*
   *
   * BUILTIN OPERATIONS
   *
   */

  either<error_info, numeric> numeric_or_error(unit_ptr const& u) noexcept {
    if (is_numeric(u->expr)) {
      return succeed(::std::get<numeric>(u->expr));
    }

    FAIL_WITH("Expected a numeric value.", u->pos);
  }

#define ARITHMETIC_OPERATOR(name, operation) \
  result_type name##_m(unit_ptr const& u, env_node_ptr&) noexcept { \
    auto const& args = as_list(u->expr).children; \
    ASSERT_ARG_COUNT(args, u->pos, >= 1); \
    auto first = numeric_or_error(args[1]); \
    RETURN_IF_ERROR(first); \
    numeric result = first.value(); \
    for (::std::size_t idx = 2; idx < args.size(); ++idx) { \
      auto noe = numeric_or_error(args[idx]); \
      RETURN_IF_ERROR(noe); \
      result operation##= noe.value(); \
    } \
    SUCCEED_WITH(u->pos, result); \
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

  result_type eval_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;

    ASSERT_ARG_COUNT(args, u->pos, == 1);
    Q_OR_ERROR(args[1]);

    return eval(args[1], node, true);
  }

  result_type list_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;
    ASSERT_ARG_COUNT(args, u->pos, >= 1);
    SUCCEED_WITH(u->pos, (list{true, {args.begin() + 1, args.end(), &mem_pool}}));
  }

  result_type head_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;
    ASSERT_ARG_COUNT(args, u->pos, == 1);

    bool q;

    if (!(q = is_q(args[1])) && !is_raw(args[1])) {
      FAIL_WITH("Head expects a Q expression or a raw string as an argument.", 
                args[1]->pos);
    }

    if (q) {
      auto const& ls = as_list(args[1]->expr);
      auto const empty = ls.children.empty();
      SUCCEED_WITH(
        empty ? args[1]->pos : ls.children.front()->pos,
        (list{
          true, 
          empty 
            ? list::children_type{&mem_pool} 
            : list::children_type{
              ls.children.begin(), ls.children.begin() + 1, &mem_pool
            }
        })
      );
    }

    auto const& str = as_string(args[1]->expr).str;

    SUCCEED_WITH(
      args[1]->pos,
      (string{
        .str = str.empty() 
          ? string_representation{"", &mem_pool} 
          : string_representation{str.begin(), str.begin() + 1, &mem_pool},
        .raw = true
      })
    );
  }

  result_type tail_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;
    ASSERT_ARG_COUNT(args, u->pos, == 1);

    bool q;

    if (!(q = is_q(args[1])) && !is_raw(args[1])) {
      FAIL_WITH("Tail expects a Q expression or a raw string as an argument.", 
                args[1]->pos);
    }

    if (q) {
      auto const& ls = as_list(args[1]->expr);
      SUCCEED_WITH(args[1]->pos, (list{
        true,
        {
          ls.children.begin() + 1, 
          ls.children.empty() 
            ? ls.children.begin() + 1
            : ls.children.end(),
          &mem_pool
        }
      }));
    }

    auto const& str = as_string(args[1]->expr).str;

    SUCCEED_WITH(
      args[1]->pos,
      (string{
        .str = {
          str.begin() + 1, 
          str.empty() ? str.begin() + 1 : str.end(), 
          &mem_pool
        },
        .raw = true
      })
    );
  }

  result_type last_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;
    ASSERT_ARG_COUNT(args, u->pos, == 1);

    bool q;

    if (!(q = is_q(args[1])) && !is_raw(args[1])) {
      FAIL_WITH("Head expects a Q expression or a raw string as an argument.", 
                args[1]->pos);
    }

    if (q) {
      auto const& ls = as_list(args[1]->expr);
      auto const empty = ls.children.empty();
      SUCCEED_WITH(empty ? args[1]->pos : ls.children.back()->pos, (list{
        true,
        {
          ls.children.end() - !empty, 
          ls.children.end(),
          &mem_pool
        }
      }));
    }

    auto const& str = as_string(args[1]->expr).str;

    SUCCEED_WITH(
      args[1]->pos,
      (string{
        .str = {
          str.end() - !str.empty(), 
          str.end(), 
          &mem_pool
        },
        .raw = true
      })
    );
  }

  result_type join_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;
    ASSERT_ARG_COUNT(args, u->pos, >= 1);

    bool q;

    if (!(q = is_q(args[1])) && !is_raw(args[1])) {
      FAIL_WITH("Tail expects a Q expression or a raw string as an argument.", 
                args[1]->pos);
    }

    if (q) {
      auto ret = list{true};
      for (::std::size_t i = 1; i < args.size(); ++i) {
        Q_OR_ERROR(args[i]);
        auto const& other = as_list(args[i]->expr);
        ret.children.insert(ret.children.end(), 
                            other.children.begin(), other.children.end());
      }
      SUCCEED_WITH(u->pos, ::std::move(ret));
    }

    string str;
    str.raw = true;
    for (::std::size_t i = 1; i < args.size(); ++i) {
      RAW_OR_ERROR(args[i]);
      str.str += as_string(args[i]->expr).str;
    }

    SUCCEED_WITH(u->pos, std::move(str));
  }

  result_type cons_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;

    ASSERT_ARG_COUNT(args, u->pos, == 2);
    Q_OR_ERROR(args[2]);

    list other = as_list(args[2]->expr);
    other.children.insert(other.children.begin(), args[1]);

    SUCCEED_WITH(u->pos, ::std::move(other));
  }

  result_type len_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;

    ASSERT_ARG_COUNT(args, u->pos, == 1);

    bool q;
    if (!(q = is_q(args[1])) && !is_raw(args[1])) {
      FAIL_WITH("Expected a Q expression or a raw string.", args[1]->pos);
    }

    SUCCEED_WITH(
      u->pos, 
      numeric(q 
        ? as_list(args[1]->expr).children.size() 
        : as_string(args[1]->expr).str.size())
    );
  }

  result_type init_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;
    ASSERT_ARG_COUNT(args, u->pos, == 1);

    bool q;

    if (!(q = is_q(args[1])) && !is_raw(args[1])) {
      FAIL_WITH("Tail expects a Q expression or a raw string as an argument.", 
                args[1]->pos);
    }

    if (q) {
      auto const& ls = as_list(args[1]->expr);
      SUCCEED_WITH(
        args[1]->pos,
        (list{
          true, 
          {
            ls.children.begin(), 
            ls.children.empty() 
              ? ls.children.begin()
              : ls.children.end() - 1,
            &mem_pool
          } 
        })
      );
    }

    auto const& str = as_string(args[1]->expr).str;

    SUCCEED_WITH(
      args[1]->pos,
      (string{
        .str = {
          str.begin(), str.empty() ? str.begin() : str.end() - 1, &mem_pool
        },
        .raw = true
      })
    );
  }
 
  result_type def_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;
  
    ASSERT_ARG_COUNT(args, u->pos, >= 2);
    Q_OR_ERROR(args[1]);

    auto const& arguments = as_list(args[1]->expr).children;

    auto g_env = global_environment();

    if (arguments.size() != args.size() - 2) {
      FAIL_WITH(
        "Differing length of arguments and corresponding assignments.",
        u->pos
      );
    }

    for (::std::size_t i = 0; i < arguments.size(); ++i) {
      if (!is_string(arguments[i]->expr)
          || as_string(arguments[i]->expr).raw) {
        FAIL_WITH("Unexpected non-symbol in the argument list.", 
                  arguments[i]->pos);
      }
      (*g_env->curr)[as_string(arguments[i]->expr).str] = args[2 + i];
    }

    SUCCEED_WITH(u->pos, (list{}));
  }

  result_type assignment_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;
  
    ASSERT_ARG_COUNT(args, u->pos, >= 2);
    Q_OR_ERROR(args[1]);

    auto const& arguments = as_list(args[1]->expr).children;

    if (arguments.size() != args.size() - 2) {
      FAIL_WITH(
        "Differing length of arguments and corresponding assignments.",
        u->pos
      );
    }

    for (::std::size_t i = 0; i < arguments.size(); ++i) {
      if (!is_string(arguments[i]->expr)
          || as_string(arguments[i]->expr).raw) {
        FAIL_WITH("Unexpected non-symbol in the argument list.", arguments[i]->pos);
      }
      (*node->curr)[as_string(arguments[i]->expr).str] = args[2 + i];
    }

    SUCCEED_WITH(u->pos, (list{}));
  }

  function::type create_function(
    bool const variadic, bool const unused,
    list const& arglist, unit_ptr const& body,
    env_ptr self_env = {}
  ) noexcept {
    return [=](unit_ptr const& u, env_node_ptr& private_env) mutable -> result_type {
      auto const& arguments = as_list(u->expr).children;
      if (!variadic && arglist.children.size() < arguments.size() - 1) {
        FAIL_WITH(
          concat(
            "Excess arguments, expected ",
            arglist.children.size(),
            ", got ",
            arguments.size() - 1,
            "."
          ),
          u->pos
        );
      }

      bool partial = !variadic && arglist.children.size() > arguments.size() - 1;

      // TODO: does it work without this?
      // also evaluate the line above
      if (variadic 
          && arglist.children.size() > arguments.size() + !unused - 1) {
        FAIL_WITH(
          "Not enough values to assign to non-variadic parameters.",
          u->pos
        );
      }

      self_env = self_env ?: make_shared<environment>();

      for (::std::size_t i = 0; 
           i != (arglist.children.size() ? arguments.size() - 1 : 0); ++i) {
        auto sym = ::std::get<string>(arglist.children[i]->expr);
        if (sym.str[0] == '&') {
          if (unused) {
            break;
          }
        
          (*self_env)[::std::get<string>(arglist.children[i + 1]->expr).str] = 
            make_shared<unit>(
              arguments[i + 1]->pos,
              list{
                true, 
                {arguments.begin() + i + 1, arguments.end(), &mem_pool}
              }
            );
          break;
        }

        (*self_env)[::std::get<string>(arglist.children[i]->expr).str] = 
          arguments[i + 1];
      }

      if (partial) {
        SUCCEED_WITH(body->pos, (function{
          .description = "User defined partially evaluated function.",
          .func = [=](unit_ptr const& nested_u, env_node_ptr& nested_env) -> result_type {
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
              self_env
            )(nested_u, nested_env);
          }})
        );
      }

      return eval(
        body, 
        make_shared<env_node>(env_node{
          .curr = self_env,
          .prev = private_env
        }), 
        true
      );
    };
  }

  result_type lambda_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;

    ASSERT_ARG_COUNT(args, u->pos, >= 2);
    ASSERT_ARG_COUNT(args, u->pos, <= 3);

    Q_OR_ERROR(args[1]);

    string_representation doc_string{"User defined function.", &mem_pool};

    if (args.size() == 4) {
      Q_OR_ERROR(args[3]);
      if (!is_string(args[2]->expr) || !as_string(args[2]->expr).raw) {
        FAIL_WITH("Expected a raw doc-string.", args[2]->pos);
      }
      doc_string = as_string(args[2]->expr).str;
    } else {
      Q_OR_ERROR(args[2]);
    }

    auto const& arglist = as_list(args[1]->expr);

    bool variadic = arglist.children.size() == 0;
    bool unused   = variadic;

    for (::std::size_t i = 0; i < arglist.children.size(); ++i) {
      if (!is_string(arglist.children[i]->expr) 
          || ::std::get<string>(arglist.children[i]->expr).raw) {
        FAIL_WITH("Expected a symbol.", arglist.children[i]->pos);   
      }

      if (::std::get<string>(arglist.children[i]->expr).str[0] == '&') {
        if (variadic) {
          FAIL_WITH(
            "Can not have more than one variadic sign.", arglist.children[i]->pos
          );
        }
        variadic = true;
        if (arglist.children.size() - i > 2) {
          FAIL_WITH(
            "Variadic sign expects either zero or one argument.",
            arglist.children[i]->pos
          );
        }
        unused = i == arglist.children.size() - 1;
      }
    }

    auto const& body = args.size() == 4 ? args[3] : args[2];

    SUCCEED_WITH(
      u->pos,
      (function{
        .description = ::std::move(doc_string),
        .func = create_function(variadic, unused, arglist, body)
      })
    );
  }

  result_type help_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr).children;
    
    ASSERT_ARG_COUNT(args, u->pos, <= 1);

    string s{{"\n", &mem_pool}, true};

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

      SUCCEED_WITH(u->pos, ::std::move(s));
    }

    expression expr;

    if (is_string(args[1]->expr)) {
      auto resolved = resolve_symbol(args[1], env); 
      RETURN_IF_ERROR(resolved);

      expr = (*env->curr)[::std::get<string>(resolved.value()->expr).str]->expr;
    } else {
      expr = args[1]->expr;
    }

    s.str += "  type: ";
    s.str += type_of(expr);
    s.str += "\n";

    s.str += "  value: ";

    ::std::stringstream ss;
    ss << expr << "\n";
    s.str += ss.str();
    
    SUCCEED_WITH(u->pos, ::std::move(s));
  }

  /*
   *
   * COMPARISON AND ORDERING
   *
   */

  either<error_info, bool> operator==(unit_ptr const& a, unit_ptr const& b) noexcept {
    if (a.get() == b.get()) {
      return succeed(true);
    }

    if (a->expr.index() != b->expr.index()) {
      return succeed(false);
    }

    if (is_numeric(a->expr)) {
      return succeed(::std::get<numeric>(a->expr) == ::std::get<numeric>(b->expr));
    }

    if (is_string(a->expr)) {
      auto const& sa = ::std::get<string>(a->expr);
      auto const& sb = ::std::get<string>(b->expr);
      return succeed(sa.raw == sb.raw && sa.str == sb.str);
    }

    if (is_list(a->expr)) {
      auto const& al = as_list(a->expr);
      auto const& bl = as_list(b->expr);

      return succeed(al.q == bl.q && al.children == bl.children);
    }
    
    FAIL_WITH("Function comparison is unsupported.", a->pos);
  }

  result_type equal_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr).children;
    ASSERT_ARG_COUNT(args, u->pos, == 2);
    auto ret = args[1] == args[2];
    RETURN_IF_ERROR(ret); 
    SUCCEED_WITH(u->pos, static_cast<numeric>(ret.value()));
  }


  result_type not_equal_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto ret = equal_m(u, env);
    RETURN_IF_ERROR(ret);
    SUCCEED_WITH(u->pos, !::std::get<numeric>(ret.value()->expr));
  }

#define SIMPLE_ORDERING(name, op) \
  result_type name##_m(unit_ptr const& u, env_node_ptr& env) noexcept { \
    auto const& args = as_list(u->expr).children; \
    ASSERT_ARG_COUNT(args, u->pos, == 2); \
    if (args[1]->expr.index() != args[1]->expr.index() \
        || !is_numeric(args[1]->expr)) { \
      FAIL_WITH("Expected two numeric arguments", u->pos);  \
    } \
    SUCCEED_WITH(u->pos, static_cast<numeric>( \
      ::std::get<numeric>(args[1]->expr) op ::std::get<numeric>(args[2]->expr) \
    )); \
  }

  SIMPLE_ORDERING(less_than, <);
  SIMPLE_ORDERING(greater_than, >);
  SIMPLE_ORDERING(less_or_equal, <=);
  SIMPLE_ORDERING(greater_or_equal, >=);

  result_type if_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr).children;

    ASSERT_ARG_COUNT(args, u->pos, >= 2);
    ASSERT_ARG_COUNT(args, u->pos, <= 3);

    bool has_else = args.size() == 4;

    if (!is_numeric(args[1]->expr)) {
      FAIL_WITH("Expected a numeric value.", u->pos);
    }

    if (!::std::get<numeric>(args[1]->expr)) {
      if (has_else) {
        return eval(args[3], env, true);
      }
      SUCCEED_WITH(u->pos, list{.q = true});
    } else {
      return eval(args[2], env, true);
    }
  }

  result_type readlines_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr).children;
    ASSERT_ARG_COUNT(args, u->pos, == 1);
    RAW_OR_ERROR(args[1]);

    ::std::ifstream in{as_string(args[1]->expr).str.c_str()};

    if (!in.is_open()) {
      FAIL_WITH("Unable to open given file.", args[1]->pos);
    }

    list lines{.q = true};

    string_representation line{&mem_pool};
    while (::std::getline(in, line)) {
      lines.children.push_back(
        make_shared<unit>(args[1]->pos, string{::std::move(line), true}));  
    }

    SUCCEED_WITH(args[1]->pos, ::std::move(lines));
  }

   
  env_node_ptr global_environment() noexcept {
    auto init = [] {
      auto g_env = make_shared(environment{&mem_pool});

      ::std::initializer_list<::std::pair<string_representation, unit_ptr>> list = 
        {{"+", make_shared<unit>(unit{{0, 0}, function{{"Adds numbers.", &mem_pool}, add_m}})},
        {"-", make_shared<unit>(unit{{0, 0}, function{{"Subtracts numbers.", &mem_pool}, sub_m}})},
        {"*", make_shared<unit>(unit{{0, 0}, function{{"Multiplies numbers.", &mem_pool}, mul_m}})},
        {"/", make_shared<unit>(unit{{0, 0}, function{{"Divides numbers.", &mem_pool}, div_m}})},
        {"%", make_shared<unit>(unit{{0, 0}, function{{"Modulo.", &mem_pool}, mod_m}})},
        {"&", make_shared<unit>(unit{{0, 0}, function{{"Binary and.", &mem_pool}, and_m}})},
        {"|", make_shared<unit>(unit{{0, 0}, function{{"Binary or.", &mem_pool}, or_m}})},
        {"^", make_shared<unit>(unit{{0, 0}, function{{"Binary xor.", &mem_pool}, xor_m}})},
        {"<<", make_shared<unit>(unit{{0, 0}, function{{"Shift left.", &mem_pool}, shl_m}})},
        {">>", make_shared<unit>(unit{{0, 0}, function{{"Shift right.", &mem_pool}, shr_m}})},
        {"eval", make_shared<unit>(unit{{0, 0}, function{{"Evaluates a Q expression.", &mem_pool}, eval_m}})},
        {"list", make_shared<unit>(unit{{0, 0}, function{{"Takes arguments and turns them into a Q expression.", &mem_pool}, list_m}})},
        {"head", make_shared<unit>(unit{{0, 0}, function{
          "Takes a Q expression or a raw string and returns the first subexpression.", head_m
        }})},
        {"tail", make_shared<unit>(unit{{0, 0}, function{
          "Takes a Q expression or a raw string and returns it without its 1st element.", tail_m
        }})},
        {"last", make_shared<unit>(unit{{0, 0}, function{
          "Takes a Q expression or a raw string and returns its last element.",
          last_m
        }})},
        {"join", make_shared<unit>(unit{{0, 0}, function{{"Joins one or more Q expressions or raw strings.", &mem_pool}, join_m}})},
        {"cons", make_shared<unit>(unit{{0, 0}, function{{"Appends its first argument to the second Q expression.", &mem_pool}, cons_m}})},
        {"len", make_shared<unit>(unit{{0, 0}, function{{"Calculates the length of a Q expression or a raw string.", &mem_pool}, len_m}})},
        {"init", make_shared<unit>(unit{{0, 0}, function{{"Returns a Q expression or a raw string without it's last element.", &mem_pool}, init_m}})},
        {"def", make_shared<unit>(unit{{0, 0}, function{
          "Global assignment to symbols in a Q expression. "
          "For example 'def {a b} 1 2' assigns 1 and 2 to a and b.",
          def_m
        }})},
        {"=", make_shared<unit>(unit{{0, 0}, function{
          "Local assignment to symbols in a Q expression. "
            "'= {x} 1' assigns 1 to x in a local scope.",
          assignment_m
        }})},
        {"\\", make_shared<unit>(unit{{0, 0}, function{
          "Lambda function, takes a Q expression argument list, "
          "and a Q expression body. "
          "For example '(\\{x y} {+ x y}) 2 3' will yield 5",
          lambda_m
        }})},
        {"help", make_shared<unit>(unit{{0, 0}, function{{"Outputs information about a symbol.", &mem_pool}, help_m}})},
        {"==", make_shared<unit>(unit{{0, 0}, function{
          "Compares arguments for equality. Can not compare functions.",
          equal_m
        }})},
        {"!=", make_shared<unit>(unit{{0, 0}, function{
          "Compares arguments for non equality. Can not compare functions.",
          not_equal_m
        }})},
        {"<", make_shared<unit>(unit{{0, 0}, function{{"Tests if first number is less than second.", &mem_pool}, less_than_m}})},
        {">", make_shared<unit>(unit{{0, 0}, function{{"Tests if first number is greater than second.", &mem_pool}, greater_than_m}})},
        {"<=", make_shared<unit>(unit{{0, 0}, function{{"Tests if first number is less than or equal to second.", &mem_pool}, less_or_equal_m}})},
        {">=", make_shared<unit>(unit{{0, 0}, function{{"Tests if first number is greater than or equal to second.", &mem_pool}, greater_or_equal_m}})},
        {"if", make_shared<unit>(unit{{0, 0}, function{
          "If(-else) statement. Takes a numeric for test. "
          "If it is != 0 it executes the first given Q expression. "
          "Otherwise, if it exists, it executes the second Q expression.",
          if_m
        }})},
        {"readlines", make_shared<unit>(unit{{0, 0}, function{
          "Returns a Q expression of lines from a file.",
          readlines_m
        }})
      }};

      for (auto& [a, b] : list) {
        g_env->operator[](a) = b;
      }

      return g_env;

    };

    auto static g_env = init();

    return make_shared<env_node>(env_node{
      .curr = g_env,
      .prev = {}
    });
  }

  /*
   *
   * EVAL FUNCTIONS
   *
   */

  result_type resolve_symbol(unit_ptr const& pu, env_node_ptr node) noexcept {
    if (!is_string(pu->expr)) {
      FAIL_WITH("Expected a symbol.", pu->pos);
    }

    auto starting_point = node;
    auto s = ::std::get<string>(pu->expr).str;

    for (;;) {
      auto iter = node->curr->begin();
      while ((iter = node->curr->find(s)) == node->curr->end()) {
        if (!node->prev) {
          FAIL_WITH(concat("Symbol ", s, " is undefined."), pu->pos); 
        }
        node = node->prev;
      }

      auto next = iter->second;

      if (!is_string(next->expr) || as_string(next->expr).raw) {
        return succeed(make_shared(unit{pu->pos, next->expr}));
      }

      s = as_string(next->expr).str;
      node = starting_point;
    }
  }

  result_type eval(
    unit_ptr const& pu, 
    env_node_ptr node, 
    bool const force_eval
  ) noexcept {
    if (is_numeric(pu->expr)) {
      return succeed(pu);
    }

    if (is_string(pu->expr)) {
      if (::std::get<string>(pu->expr).raw) {
        return succeed(pu);
      }

      if (auto rsr = resolve_symbol(pu, node); rsr) {
        return succeed(rsr.value());
      } else {
        return fail(rsr.error());
      }
    }

    if (is_list(pu->expr)) {
      auto ls = as_list(pu->expr);

      if (!force_eval && ls.q) {
        return succeed(pu);
      }

      if (ls.children.empty()) {
        return succeed(pu);
      }
      
      for (auto& child : ls.children) {
        auto new_child = eval(child, node);
        RETURN_IF_ERROR(new_child);
        child = new_child.value();
      }

      if (ls.children.size() == 1 && !is_function(ls.children.front()->expr)) {
        return succeed(ls.children.front());
      }

      if (!is_function(ls.children.front()->expr)) {
        FAIL_WITH(
          "Expected a builtin or user defined function.",
          ls.children.front()->pos
        );
      }

      auto const& fn = ::std::get<function>(ls.children.front()->expr);
      return fn.func(make_shared<unit>(pu->pos, ls), node);
    }

    if (is_function(pu->expr)) {
      return succeed(pu);
    }

    terminate_with("Missing a type check in eval.");
    __builtin_unreachable();
  }

}
