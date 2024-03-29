#pragma once

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <yl/mem.hpp>
#include <yl/util.hpp>
#include <yl/types.hpp>
#include <yl/eval.hpp>
#include <yl/type_operations.hpp>

namespace yl {

#define ASSERT_ARG_COUNT(u, eq) \
  if (!(as_list(u->expr).size() - 1 eq)) { \
    FAIL_WITH("Argument count must be " #eq ".", u->pos) \
  }

  inline bool is_raw(unit_ptr const& u) noexcept {
    return is_string(u->expr) && as_string(u->expr).raw;
  }

#define NUMERIC_OR_ERROR(unit_ptr) \
  if (!is_numeric(unit_ptr->expr)) {\
    return fail(error_info{ \
      concat("Expected a number got ", type_of(unit_ptr->expr), "."), \
       unit_ptr->pos \
    });  \
  }

#define LIST_OR_ERROR(unit_ptr) \
  if (!is_list(unit_ptr->expr)) {\
    return fail(error_info{ \
      concat("Expected a Q expression got ", type_of(unit_ptr->expr), ": ", unit_ptr->expr), \
       unit_ptr->pos \
    });  \
  }

#define RAW_OR_ERROR(unit_ptr) \
  if (!is_raw(unit_ptr)) { \
    FAIL_WITH(concat("Expected a raw string got ", type_of(unit_ptr->expr), "."), \
              unit_ptr->pos); \
  }

  inline either<error_info, numeric> numeric_or_error(unit_ptr const& u) noexcept {
    if (is_numeric(u->expr)) {
      return succeed(as_numeric(u->expr));
    }

    FAIL_WITH(
      concat(
        "Expected a numeric value got ", 
        type_of(u->expr), 
        ": ",
        u->expr,
        "."
      ), u->pos);
  }

#define ARITHMETIC_OPERATOR(name, operation) \
  inline result_type name##_m(unit_ptr const& u, env_node_ptr&) noexcept { \
    auto const& args = as_list(u->expr); \
    ASSERT_ARG_COUNT(u, >= 1); \
    auto first = cast_numeric(args[1]); \
    RETURN_IF_ERROR(first); \
    numeric result = first.value(); \
    for (::std::size_t idx = 2; idx < args.size(); ++idx) { \
      auto noe = cast_numeric(args[idx]); \
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

  inline result_type quote_m(unit_ptr const& u, env_node_ptr&) noexcept {
    ASSERT_ARG_COUNT(u, == 1);
    return succeed(as_list(u->expr)[1]);
  }

  inline result_type eval_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr);

    ASSERT_ARG_COUNT(u, == 1);
    return eval(args[1], node);
  }

  inline result_type list_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr);
    ASSERT_ARG_COUNT(u, >= 1);
    SUCCEED_WITH(u->pos, (make_seq<unit_ptr>(args.begin() + 1, args.end())));
  }

  inline result_type echo_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    ASSERT_ARG_COUNT(u, == 1);
    ::std::cout << as_list(u->expr)[1]->expr << "\n";
    SUCCEED_WITH(u->pos, make_list());
  }

#define SINGLE_LIST_BUILTIN(name, q_expr, r_string) \
  inline result_type name##_m(unit_ptr const& u, env_node_ptr& node) noexcept { \
    auto const& args = as_list(u->expr); \
    ASSERT_ARG_COUNT(u, == 1); \
    bool is_ls = is_list(args[1]->expr); \
    if (!is_ls && !is_raw(args[1])) { \
      FAIL_WITH("Expected a list or a raw string as an argument.", \
                args[1]->pos); \
    } \
    return is_ls ? q_expr(args[1]) : r_string(args[1]); \
  }

  SINGLE_LIST_BUILTIN(
    head,
    [](auto const& u) { 
      auto const& ls = as_list(u->expr);
      auto const empty = ls.empty();
      SUCCEED_WITH(
        empty ? u->pos : ls.front()->pos,
          empty 
            ? expression{make_list()}
            : ls[0]->expr
      );
    },
    [](auto const& u) {
      auto const& str = as_string(u->expr).str;
      SUCCEED_WITH(
        u->pos,
        (string{
          .str = str.empty() 
            ? make_string()
            : make_string(str.begin(), str.begin() + 1),
          .raw = true
        })
      );
    }
  ); 

  SINGLE_LIST_BUILTIN(
    tail,
    [](auto const& u) {
      auto const& ls = as_list(u->expr);
      SUCCEED_WITH(u->pos,
        make_seq<unit_ptr>(
          ls.begin() + 1, 
          ls.empty() 
            ? ls.begin() + 1
            : ls.end()
        )
      );
    },
    [](auto const& u) {
      auto const& str = as_string(u->expr).str;
      SUCCEED_WITH(
        u->pos,
        (string{
          .str = make_string(
            str.begin() + 1, 
            str.empty() ? str.begin() + 1 : str.end() 
          ),
          .raw = true
        })
      );
    }
  );
  
  SINGLE_LIST_BUILTIN(
    last,
    [](auto const& u) {
      auto const& ls = as_list(u->expr);
      auto const empty = ls.empty();
      SUCCEED_WITH(empty ? u->pos : ls.back()->pos,
        empty ? expression{make_list()} : ls.back()->expr
      );
    },
    [](auto const& u) {
      auto const& str = as_string(u->expr).str;
      SUCCEED_WITH(
        u->pos,
        (string{
          .str = make_string(
            str.end() - !str.empty(), 
            str.end()
          ),
          .raw = true
        })
      );
    }
  );

  SINGLE_LIST_BUILTIN(
    init,
    [](auto const& u) {
      auto const& ls = as_list(u->expr);
      SUCCEED_WITH(
        u->pos,
          make_seq<unit_ptr>(
            ls.begin(), 
            ls.empty() ? ls.begin() : ls.end() - 1
          )
      );
    },
    [](auto const& u) {
      auto const& str = as_string(u->expr).str;
      SUCCEED_WITH(
        u->pos,
        (string{
          .str = make_string(
            str.begin(), str.empty() ? str.begin() : str.end() - 1
          ),
          .raw = true
        })
      );
    }
  );

  inline result_type join_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr);
    ASSERT_ARG_COUNT(u, >= 1);

    bool is_ls;

    if (!(is_ls = is_list(args[1]->expr)) && !is_raw(args[1])) {
      FAIL_WITH("Join expects a list expression or a raw string as an argument.", 
                args[1]->pos);
    }

    if (is_ls) {
      auto ret = make_list();
      for (::std::size_t i = 1; i < args.size(); ++i) {
        auto const& other = as_list(args[i]->expr);
        ret.insert(ret.end(), 
                            other.begin(), other.end());
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

  inline result_type cons_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr);

    ASSERT_ARG_COUNT(u, == 2);
    return cast_list(args[2]).collect_flat(
      [&](auto&&) {
        return cast_hash_map(args[2]).collect_flat(
          [&](auto&&) { 
            FAIL_WITH(
              concat(
                "Expected Q expression or hash_map, got: ", 
                type_of(args[2]->expr), " ", args[2]->expr), 
              args[2]->pos); 
          },
          [&](auto&& map) {
            return cast_list(args[1]).collect_flat(
              fail_functor,
              [&](auto&& q) -> result_type {
                if (q.size() != 2) {
                  FAIL_WITH("Expected a Q expression with two elements.", args[1]->pos);
                }
                SUCCEED_WITH(unit{
                  u->pos,
                  expression{map.insert({q[0], q[1]})}});
              }
            );
          }
        );
      },
      [&](auto other) {
        other.insert(other.begin(), args[1]);
        SUCCEED_WITH(u->pos, ::std::move(other));
      }
    );
  }

  inline result_type at_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr);
    ASSERT_ARG_COUNT(u, == 2);

    auto& seq = args[2];
    auto& idx = args[1];

    auto const err = fail(error_info{
      .error_message = concat(
        "Expected Q expr, raw string or hash map, got: ", 
        type_of(args[2]->expr)),
      .pos = args[2]->pos
    });

    return cast_qr(seq).collect_flat(
      [&](auto&&) {
        return cast_hash_map(args[2]).collect_flat(
          [err](auto&&) { return err; },
          [&idx, pos = u->pos](auto&& map) -> result_type { 
            if (!map.count(idx)) {
              SUCCEED_WITH(pos, make_list());  
            }
            return succeed(map.at(idx)); 
          }
        );
      },

      [&](auto&& ls_or_str) {
        return cast_numeric(args[1]).flat_map(
          [&](auto&& num_idx) -> result_type {
            if (num_idx < 0 || static_cast<::std::size_t>(num_idx) >= len(seq)) {
              FAIL_WITH(
                concat(num_idx, " is out of bounds for size ", len(seq), "."), 
                idx->pos);
            }
            return ls_or_str.collect_flat(
              [num_idx](auto&& ls) { return succeed(ls[num_idx]); },
              [&](auto&& str) { SUCCEED_WITH(
                u->pos, 
                (string{make_string(str.str.substr(num_idx, 1)), true})); 
              }
            );
          }
        );
      }
    );
  }

  inline result_type len_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr);

    ASSERT_ARG_COUNT(u, == 1);

    if (is_list(args[1]->expr)) {
      SUCCEED_WITH(u->pos, numeric(as_list(args[1]->expr).size()));
    }

    if (is_raw(args[1])) {
      SUCCEED_WITH(u->pos, numeric(as_string(args[1]->expr).str.size()));
    }

    if (is_hash_map(args[1]->expr)) {
      SUCCEED_WITH(u->pos, numeric(as_hash_map(args[1]->expr).size()));
    }

    FAIL_WITH(
      "Expected a Q expression, hash map, or raw string.", 
      args[1]->pos);
  }

  inline result_type assignment_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr);
  
    ASSERT_ARG_COUNT(u, >= 2);

    bool is_ls = is_list(args[1]->expr);
    auto ls = make_list();

    if (!is_ls) {
      if (is_raw(args[1])) {
        FAIL_WITH("Expected a symbol.", args[1]->pos);
      }
      ls.push_back(args[1]);
    }

    auto const* arguments_ptr = &ls;
    if (is_ls) {
      arguments_ptr = &as_list(args[1]->expr);
    }
    auto const& arguments = *arguments_ptr;

    auto g_env = global_environment();

    if (arguments.size() != args.size() - 2) {
      FAIL_WITH(
        concat(
          "Differing length of arguments and corresponding assignments: ",
          arguments.size(), args.size() - 2
        ),
        u->pos
      );
    }

    for (::std::size_t i = 0; i < arguments.size(); ++i) {
      if (!is_string(arguments[i]->expr)
          || as_string(arguments[i]->expr).raw) {
        FAIL_WITH("Unexpected non-symbol in the argument list.", 
                  arguments[i]->pos);
      }
      auto new_value = eval(args[2 + i], node);
      RETURN_IF_ERROR(new_value);
      (*node->curr)[as_string(arguments[i]->expr).str] = new_value.value();
    }

    SUCCEED_WITH(u->pos, (make_list()));
  }
 
  inline result_type def_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto ptr = global_environment();
    ptr->prev = node;
    return assignment_m(u, ptr);
  }


  namespace detail {

    inline result_type decompose_impl(unit_ptr const& sym, 
                                      unit_ptr const& expr,
                                      env_node_ptr& node) noexcept {
      if (is_string(sym->expr) && !as_string(sym->expr).raw) {
        (*node->curr)[as_string(sym->expr).str] = expr;
      } else if (is_list(sym->expr)) {
        LIST_OR_ERROR(expr);

        auto const& syms = as_list(sym->expr);
        auto const& sube = as_list(expr->expr);

        if (syms.size() != sube.size()) {
          FAIL_WITH("Expressions must be of equal length.", sym->pos);
        }

        for (::std::size_t i = 0; i < syms.size(); ++i) {
          RETURN_IF_ERROR(detail::decompose_impl(syms[i], sube[i], node));
        }
      } else {
        FAIL_WITH("Expected either a symbol or a sub-Q expression.", sym->pos);
      }

      SUCCEED_WITH(sym->pos, (make_list()));
    }

  }

  inline result_type decompose_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr);
    ASSERT_ARG_COUNT(u, == 2);

    auto const evald = eval(args[2], node);
    RETURN_IF_ERROR(evald);
    
    RETURN_IF_ERROR(detail::decompose_impl(args[1], evald.value(), node));
    SUCCEED_WITH(u->pos, (make_list()));
  }

  namespace detail {

    enum class function_kind {
      regular,
      macro,
      syntax
    };

  }

  inline function::type create_function(
    bool const variadic, bool const unused,
    list const& arglist, unit_ptr const& body,
    env_node_ptr closure,
    detail::function_kind const fk,
    env_ptr self_env = {}
  ) noexcept {
    return [=](unit_ptr const& u, env_node_ptr& syntax_env) mutable -> result_type {
      auto const& arguments = as_list(u->expr);
      if (!variadic && arglist.size() < arguments.size() - 1) {
        FAIL_WITH(
          concat(
            "Excess arguments, expected ",
            arglist.size(),
            ", got ",
            arguments.size() - 1,
            "."
          ),
          u->pos
        );
      }

      bool partial = !variadic && arglist.size() > arguments.size() - 1;

      // TODO: does it work without this?
      // also evaluate the line above
      if (variadic 
          && arglist.size() > arguments.size() + !unused) {
        FAIL_WITH(
          "Not enough values to assign to non-variadic parameters.",
          u->pos
        );
      }

      // 
      self_env = self_env ?: make_shared<environment>();

      for (::std::size_t i = 0; 
           i != (arglist.size() ? arguments.size() - 1 : 0); ++i) {
        auto sym = as_string(arglist[i]->expr);
        if (sym.str[0] == '&') {
          if (unused) {
            break;
          }
        
          (*self_env)[as_string(arglist[i + 1]->expr).str] = 
            ::yl::make_shared<unit>(
              arguments[i + 1]->pos,
              make_seq<unit_ptr>(arguments.begin() + i + 1, arguments.end())
            );
          break;
        }

        (*self_env)[as_string(arglist[i]->expr).str] = 
          arguments[i + 1]; // fix eval with respect to macros
      }

      if (arguments.size() - 1 < arglist.size() - variadic && !unused) {
        (*self_env)[as_string(arglist.back()->expr).str] =
          ::yl::make_shared<unit>(u->pos, make_list());
      }

      if (partial) {
        SUCCEED_WITH(body->pos, (function{
          .description = make_string("User defined partially evaluated function."),
          .func = [=](unit_ptr const& nested_u, env_node_ptr& nested_env) -> result_type {
            return create_function(
              false, false, 
              make_seq<unit_ptr>(
                arglist.begin() + arguments.size() - 1,
                arglist.end()
              ),
              body,
              fk == detail::function_kind::syntax ? syntax_env : closure,
              fk,
              self_env
            )(nested_u, nested_env);
          }})
        );
      }

      return eval(
        body, 
        make_shared<env_node>(env_node{
          .curr = self_env,
          .prev = fk == detail::function_kind::syntax ? syntax_env : closure
        }) 
      );
    };
  }

  inline result_type create_function_facade(
    unit_ptr const& u, env_node_ptr& node,
    detail::function_kind const fk
  ) noexcept {
    auto const& args = as_list(u->expr);

    ASSERT_ARG_COUNT(u, >= 2);
    ASSERT_ARG_COUNT(u, <= 3);

    LIST_OR_ERROR(args[1]);

    auto doc_string = make_string("User defined function.");

    if (args.size() == 4) {
      LIST_OR_ERROR(args[3]);
      if (!is_string(args[2]->expr) || !as_string(args[2]->expr).raw) {
        FAIL_WITH("Expected a raw doc-string.", args[2]->pos);
      }
      doc_string = as_string(args[2]->expr).str;
    } else {
      LIST_OR_ERROR(args[2]);
    }

    auto const& arglist = as_list(args[1]->expr);

    bool variadic = arglist.size() == 0;
    bool unused   = variadic;

    for (::std::size_t i = 0; i < arglist.size(); ++i) {
      if (!is_string(arglist[i]->expr) 
          || as_string(arglist[i]->expr).raw) {
        FAIL_WITH("Expected a symbol.", arglist[i]->pos);   
      }

      if (as_string(arglist[i]->expr).str[0] == '&') {
        if (variadic) {
          FAIL_WITH(
            "Can not have more than one variadic sign.", arglist[i]->pos
          );
        }
        variadic = true;
        if (arglist.size() - i > 2) {
          FAIL_WITH(
            "Variadic sign expects either zero or one argument.",
            arglist[i]->pos
          );
        }
        unused = i == arglist.size() - 1;
      }
    }

    auto const& body = args.size() == 4 ? args[3] : args[2];

    SUCCEED_WITH(
      u->pos,
      (function{
        .description = ::std::move(doc_string),
        .func = create_function(variadic, unused, arglist, body, node, fk),
        .macro = fk != detail::function_kind::regular
      })
    );
  }

  inline result_type lambda_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    return create_function_facade(u, node, detail::function_kind::regular);
  }

  inline result_type macro_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    return create_function_facade(u, env, detail::function_kind::macro);
  }

  inline result_type syntax_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    return create_function_facade(u, env, detail::function_kind::syntax);
  }

  inline result_type help_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr);
    
    ASSERT_ARG_COUNT(u, <= 1);

    string s{make_string("\n"), true};

    if (args.size() == 1) {
      s.str +=
        "  This is a lisp like intepreted language.\n"
        "  There are only 4 types: numeric, symbol, function and list.\n"
        "  Numeric type is a signed integer number such as 1, or -2444.\n"
        "  Symbol is any named value such as 'a' or 'help'.\n"
        "  List is a list of types. It can be evaluated such as '(+ 1 2)'\n"
        "  or unevaluated such as '{+ 1 2}' which can be evaluated using 'eval'.\n"
        "  Unevaluated list is also referred to as a Q expression, it is similar in\n"
        "  functionality to a lisp macro.\n"
        "  Function is a resolved symbol that represents a computation,\n"
        "  it can be created using '\\', see 'help \\'.\n"
        "  Functions support partial evaluation.\n"
        "  Everything after and including ; is considered a commment.\n"
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

      expr = (*env->curr)[as_string(resolved.value()->expr).str]->expr;
    } else {
      expr = args[1]->expr;
    }

    s.str += type_of(expr);
    s.str += ":\n";

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

  inline result_type equal_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr);
    ASSERT_ARG_COUNT(u, == 2);
    SUCCEED_WITH(u->pos, args[1] == args[2]);
  }

  inline result_type not_equal_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto ret = equal_m(u, env);
    RETURN_IF_ERROR(ret);
    SUCCEED_WITH(u->pos, !as_numeric(ret.value()->expr));
  }

#define SIMPLE_ORDERING(name, op) \
  inline result_type name##_m(unit_ptr const& u, env_node_ptr& env) noexcept { \
    auto const& args = as_list(u->expr); \
    ASSERT_ARG_COUNT(u, == 2); \
    if (args[1]->expr.index() != args[2]->expr.index()) { \
      FAIL_WITH("Expected two arguments of same type.", args[1]->pos);  \
    } \
    if (is_numeric(args[1]->expr)) { \
      SUCCEED_WITH(u->pos, static_cast<numeric>( \
        as_numeric(args[1]->expr) op as_numeric(args[2]->expr) \
      )); \
    } else if (is_raw(args[1])) { \
      SUCCEED_WITH(u->pos, static_cast<numeric>( \
        as_string(args[1]->expr).str op as_string(args[2]->expr).str \
      )); \
    } \
    FAIL_WITH("Expected either raw strings or numbers as an argument.", args[1]->pos); \
  }

  SIMPLE_ORDERING(less_than, <);
  SIMPLE_ORDERING(greater_than, >);
  SIMPLE_ORDERING(less_or_equal, <=);
  SIMPLE_ORDERING(greater_or_equal, >=);

  inline result_type if_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr);

    ASSERT_ARG_COUNT(u, >= 2);
    ASSERT_ARG_COUNT(u, <= 3);

    bool has_else = args.size() == 4;
    
    auto const condition = eval(args[1], env);
    RETURN_IF_ERROR(condition);
    auto const& inner = condition.value();

    if (!is_numeric(inner->expr)) {
      FAIL_WITH("Expected a numeric value.", args[1]->pos);
    }

    if (!as_numeric(inner->expr)) {
      if (has_else) {
        return eval(args[3], env);
      }
      SUCCEED_WITH(u->pos, make_list());
    } else {
      return eval(args[2], env);
    }
  }

  namespace detail {

    // sort functions that take a comparator than can return an error

    template<typename Iter, typename Comp>
    either<error_info, Iter> partition(Iter begin, Iter end, Comp const& cmp) 
        noexcept {
      // median as pivot
      auto mid = begin + (end-- - begin) / 2;

      auto cond = cmp(*begin, *mid);
      RETURN_IF_ERROR(cond);
      if (as_numeric(cond.value()->expr)) ::std::swap(*begin, *mid);
      
      cond = cmp(*end, *mid);
      RETURN_IF_ERROR(cond);
      if (as_numeric(cond.value()->expr)) ::std::swap(*end, *mid);

      cond = cmp(*end, *begin);
      RETURN_IF_ERROR(cond);
      if (as_numeric(cond.value()->expr)) ::std::swap(*end, *begin);

      auto pivot = *begin;

      --begin;
      ++end;

      for (;;) {
        do {
          cond = cmp(*(++begin), pivot);
          RETURN_IF_ERROR(cond);
        } while (as_numeric(cond.value()->expr));

        do {
          cond = cmp(pivot, *(--end));
          RETURN_IF_ERROR(cond);
        } while (as_numeric(cond.value()->expr));

        if (begin >= end) {
          return succeed(++end);
        }
        ::std::swap(*begin, *end);
      }
    }

    template<typename Iter, typename Comp>
    either<error_info> quick_sort(Iter begin, Iter end, Comp const& cmp) {
      if (end - begin <= 1) {
        return succeed();
      }
      auto mid = detail::partition(begin, end, cmp);
      RETURN_IF_ERROR(mid);
      RETURN_IF_ERROR(quick_sort(begin, *mid, cmp));
      RETURN_IF_ERROR(quick_sort(*mid, end, cmp));
      return succeed();
    }

  }

  inline result_type sorted_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr);
    ASSERT_ARG_COUNT(u, >= 1);
    ASSERT_ARG_COUNT(u, <= 2);

    LIST_OR_ERROR(args[1]);

    unit_ptr ret = ::yl::make_shared<unit>(
      u->pos, 
      as_list(args[1]->expr)
    );

    auto& children = as_list(ret->expr);

    if (children.empty()) {
      return succeed(ret);
    }

    bool has_custom_fn = args.size() > 2;

    if (has_custom_fn && !is_function(args[2]->expr)) {
      FAIL_WITH("Expected a comparison function.", args[2]->pos);
    }

    auto const& sort_fn = 
      has_custom_fn ? as_function(args[2]->expr).func : less_than_m;

    auto err = detail::quick_sort(
      children.begin(), children.end(),
      [&](unit_ptr a, unit_ptr b) {
        auto children = make_seq<unit_ptr>();

        children.push_back(a);
        children.push_back(a);
        children.push_back(b);

        return sort_fn(::yl::make_shared<unit>(
          u->pos, 
          ::std::move(children)
        ), env);
      }
    );

    RETURN_IF_ERROR(err);
    return succeed(ret);
  }

  inline result_type stoi_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr);
    ASSERT_ARG_COUNT(u, == 1);
    RAW_OR_ERROR(args[1]);

    auto const& str = as_string(args[1]->expr).str;

    // non null char** required for strtoll
    char* eptr = reinterpret_cast<char*>(1);
    char const* sptr = str.c_str();

    auto const n = ::std::strtoll(sptr, &eptr, 10);

    if (eptr > sptr) {
      if (eptr != sptr + str.size()) {
        FAIL_WITH(
          "Invalid number format. Expected a signed integer.", 
          args[1]->pos
        );
      } else if (errno == ERANGE) {
        errno = 0;
        FAIL_WITH(
          "Given number does not fit into a 64bit signed integer.",
          args[1]->pos
        );
      }

      SUCCEED_WITH(args[1]->pos, n);
    }

    FAIL_WITH(
      "Could not convert given number to a 64bit signed integer.", 
      args[1]->pos
    );
  }

  inline result_type str_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr);
    ASSERT_ARG_COUNT(u, == 1);
    if (is_string(args[1]->expr)) {
      return succeed(args[1]);
    }
    SUCCEED_WITH(u->pos, (string{.str = concat(args[1]->expr), .raw = true}));
  }

  inline result_type readlines_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr);
    ASSERT_ARG_COUNT(u, == 1);
    RAW_OR_ERROR(args[1]);

    ::std::ifstream in{as_string(args[1]->expr).str.c_str()};

    if (!in.is_open()) {
      FAIL_WITH("Unable to open given file.", args[1]->pos);
    }

    list lines = make_list();

    auto line = make_string();
    while (::std::getline(in, line)) {
      lines.push_back(
        make_shared<unit>(args[1]->pos, string{::std::move(line), true}));  
    }

    if (lines.size() && as_string(lines.back()->expr).str.empty()) {
      lines.pop_back();
    }

    SUCCEED_WITH(args[1]->pos, ::std::move(lines));
  }

  inline result_type split_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr);
    ASSERT_ARG_COUNT(u, == 2);
    RAW_OR_ERROR(args[1]);
    RAW_OR_ERROR(args[2]);
    
    auto const& input = as_string(args[2]->expr).str;
    auto const& delim = as_string(args[1]->expr).str;

    list ret = make_list();
    ::std::size_t last_split = 0ul;
    ::std::size_t curr;

    while ((curr = input.find(delim, last_split)) != ::std::string::npos) {
      if (curr == last_split) {
        last_split = curr + delim.length();
        continue;
      }

      ret.push_back(make_shared<unit>(
        u->pos, 
        string{
          .str = input.substr(last_split, curr - last_split),
          .raw = true
        }
      ));
      last_split = curr + delim.length();
    }

    if (last_split != input.length()) {
      ret.push_back(make_shared<unit>(
        u->pos, 
        string{
          .str = input.substr(last_split, input.length() - last_split),
          .raw = true
        }
      ));
    }

    SUCCEED_WITH(u->pos, ret);
  }

  inline result_type err_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr);
    ASSERT_ARG_COUNT(u, == 1);
    FAIL_WITH(as_string(str_m(u, env).value()->expr).str, args[1]->pos);
  }

  inline result_type mk_map_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr);
    ASSERT_ARG_COUNT(u, == 1);
    LIST_OR_ERROR(args[1]);

    auto const& mappings = as_list(args[1]->expr);
    if (mappings.size() % 2) {
      FAIL_WITH(
        "Map requires key value pairings, ie. an even number of elements.", 
        args[1]->pos);
    }

    hash_map ret;

    for (::std::size_t i = 0; i < mappings.size(); i += 2) {
      // no transient..
      ret = ret.insert({mappings[i], mappings[i + 1]});
    }

    SUCCEED_WITH(unit{u->pos, expression{ret}});
  }

  inline result_type while_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    ASSERT_ARG_COUNT(u, == 2);
    auto const& args = as_list(u->expr);

    for (;;) {
      auto const condition_either = eval(args[1], env);
      RETURN_IF_ERROR(condition_either);
      NUMERIC_OR_ERROR(condition_either.value()); 
      auto const condition = as_numeric(condition_either.value()->expr);

      if (!condition) {
        break;
      }

      auto const ret = eval(args[2], env);
      RETURN_IF_ERROR(ret);
    }    

    SUCCEED_WITH(u->pos, make_list());
  }

  inline result_type keyword_m(unit_ptr const& u, env_node_ptr&) noexcept {
    FAIL_WITH(
        "Keyword is not meant to be evaluated. "
        "It is strictly used for parser/evaluator operations.", u->pos);
  }

  inline result_type is_atom_m(unit_ptr const& u, env_node_ptr&) noexcept {
    ASSERT_ARG_COUNT(u, == 1);
    auto const& arg = as_list(u->expr)[1]->expr;
    SUCCEED_WITH(u->pos, expression{numeric{is_numeric(arg) || is_string(arg)}});
  }

  #define TYPE_CHECK_M(type) \
  inline result_type is_##type##_m(unit_ptr const& u, env_node_ptr&) noexcept { \
    ASSERT_ARG_COUNT(u, == 1); \
    SUCCEED_WITH(unit{u->pos, expression{numeric{is_##type(as_list(u->expr)[1]->expr)}}}); \
  }   

  TYPE_CHECK_M(numeric);
  TYPE_CHECK_M(list);
  TYPE_CHECK_M(hash_map);
  TYPE_CHECK_M(function);

  #define TYPE_CHECK_SPECIFIC(type) \
  inline result_type is_##type##_m(unit_ptr const& u, env_node_ptr&) noexcept { \
    ASSERT_ARG_COUNT(u, == 1); \
    SUCCEED_WITH(unit{u->pos, expression{numeric{is_##type(as_list(u->expr)[1])}}}); \
  }   

  TYPE_CHECK_SPECIFIC(raw);

  inline result_type time_ms_m(unit_ptr const& u, env_node_ptr&) noexcept {
    auto const duration = 
      ::std::chrono::high_resolution_clock::now().time_since_epoch();
    auto const millis =
      ::std::chrono::duration_cast<::std::chrono::milliseconds>(duration)
        .count();

    SUCCEED_WITH(u->pos, millis);
  }

  inline result_type is_null_m(unit_ptr const& u, env_node_ptr&) noexcept {
    return cast_list(u).collect(
      [u](auto&&) { return make_shared<unit>(u->pos, false); },
      [u](auto&& l) { return make_shared<unit>(u->pos, false);}
    );
  }

}
