#pragma once

#include <fstream>

#include <yl/types.hpp>
#include <yl/eval.hpp>
#include <yl/macros.hpp>

namespace yl {

#define ASSERT_ARG_COUNT(u, eq) \
  if (!(as_list(u->expr).children.size() - 1 eq)) { \
    FAIL_WITH("Argument count must be " #eq ".", u->pos) \
  }

  inline bool is_q(unit_ptr const& u) noexcept {
    return is_list(u->expr) && as_list(u->expr).q;
  }

  inline bool is_raw(unit_ptr const& u) noexcept {
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

  inline either<error_info, numeric> numeric_or_error(unit_ptr const& u) noexcept {
    if (is_numeric(u->expr)) {
      return succeed(as_numeric(u->expr));
    }

    FAIL_WITH("Expected a numeric value.", u->pos);
  }

#define ARITHMETIC_OPERATOR(name, operation) \
  inline result_type name##_m(unit_ptr const& u, env_node_ptr&) noexcept { \
    auto const& args = as_list(u->expr).children; \
    ASSERT_ARG_COUNT(u, >= 1); \
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

  inline result_type eval_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;

    ASSERT_ARG_COUNT(u, == 1);
    Q_OR_ERROR(args[1]);

    return eval(args[1], node, true);
  }

  inline result_type list_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;
    ASSERT_ARG_COUNT(u, >= 1);
    SUCCEED_WITH(u->pos, (list{true, {args.begin() + 1, args.end(), &mem_pool}}));
  }

#define SINGLE_LIST_BUILTIN(name, q_expr, r_string) \
  inline result_type name##_m(unit_ptr const& u, env_node_ptr& node) noexcept { \
    auto const& args = as_list(u->expr).children; \
    ASSERT_ARG_COUNT(u, == 1); \
    bool q; \
    if (!(q = is_q(args[1])) && !is_raw(args[1])) { \
      FAIL_WITH("Expected a Q expression or a raw string as an argument.", \
                args[1]->pos); \
    } \
    return q ? q_expr(args[1]) : r_string(args[1]); \
  }

  SINGLE_LIST_BUILTIN(
    head,
    [](auto const& u) { 
      auto const& ls = as_list(u->expr);
      auto const empty = ls.children.empty();
      SUCCEED_WITH(
        empty ? u->pos : ls.children.front()->pos,
        (list{
          true, 
          empty 
            ? list::children_type{&mem_pool} 
            : list::children_type{
              ls.children.begin(), ls.children.begin() + 1, &mem_pool
            }
        })
      );
    },
    [](auto const& u) {
      auto const& str = as_string(u->expr).str;
      SUCCEED_WITH(
        u->pos,
        (string{
          .str = str.empty() 
            ? string_representation{"", &mem_pool} 
            : string_representation{str.begin(), str.begin() + 1, &mem_pool},
          .raw = true
        })
      );
    }
  ); 

  SINGLE_LIST_BUILTIN(
    tail,
    [](auto const& u) {
      auto const& ls = as_list(u->expr);
      SUCCEED_WITH(u->pos, (list{
        true,
        {
          ls.children.begin() + 1, 
          ls.children.empty() 
            ? ls.children.begin() + 1
            : ls.children.end(),
          &mem_pool
        }
      }));
    },
    [](auto const& u) {
      auto const& str = as_string(u->expr).str;
      SUCCEED_WITH(
        u->pos,
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
  );
  
  SINGLE_LIST_BUILTIN(
    last,
    [](auto const& u) {
      auto const& ls = as_list(u->expr);
      auto const empty = ls.children.empty();
      SUCCEED_WITH(empty ? u->pos : ls.children.back()->pos, (list{
        true,
        {
          ls.children.end() - !empty, 
          ls.children.end(),
          &mem_pool
        }
      }));
    },
    [](auto const& u) {
      auto const& str = as_string(u->expr).str;
      SUCCEED_WITH(
        u->pos,
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
  );

  SINGLE_LIST_BUILTIN(
    init,
    [](auto const& u) {
      auto const& ls = as_list(u->expr);
      SUCCEED_WITH(
        u->pos,
        (list{
          true, 
          {
            ls.children.begin(), 
            ls.children.empty() ? ls.children.begin() : ls.children.end() - 1,
            &mem_pool
          } 
        })
      );
    },
    [](auto const& u) {
      auto const& str = as_string(u->expr).str;
      SUCCEED_WITH(
        u->pos,
        (string{
          .str = {
            str.begin(), str.empty() ? str.begin() : str.end() - 1, &mem_pool
          },
          .raw = true
        })
      );
    }
  );

  inline result_type join_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;
    ASSERT_ARG_COUNT(u, >= 1);

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

  inline result_type cons_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;

    ASSERT_ARG_COUNT(u, == 2);
    Q_OR_ERROR(args[2]);

    list other = as_list(args[2]->expr);
    other.children.insert(other.children.begin(), args[1]);

    SUCCEED_WITH(u->pos, ::std::move(other));
  }

  inline result_type len_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;

    ASSERT_ARG_COUNT(u, == 1);

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
 
  inline result_type def_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;
  
    ASSERT_ARG_COUNT(u, >= 2);
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

  inline result_type assignment_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;
  
    ASSERT_ARG_COUNT(u, >= 2);
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

  inline function::type create_function(
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
        auto sym = as_string(arglist.children[i]->expr);
        if (sym.str[0] == '&') {
          if (unused) {
            break;
          }
        
          (*self_env)[as_string(arglist.children[i + 1]->expr).str] = 
            make_shared<unit>(
              arguments[i + 1]->pos,
              list{
                true, 
                {arguments.begin() + i + 1, arguments.end(), &mem_pool}
              }
            );
          break;
        }

        (*self_env)[as_string(arglist.children[i]->expr).str] = 
          arguments[i + 1];
      }

      if (partial) {
        SUCCEED_WITH(body->pos, (function{
          .description = {"User defined partially evaluated function.", &mem_pool},
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

  inline result_type lambda_m(unit_ptr const& u, env_node_ptr& node) noexcept {
    auto const& args = as_list(u->expr).children;

    ASSERT_ARG_COUNT(u, >= 2);
    ASSERT_ARG_COUNT(u, <= 3);

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
          || as_string(arglist.children[i]->expr).raw) {
        FAIL_WITH("Expected a symbol.", arglist.children[i]->pos);   
      }

      if (as_string(arglist.children[i]->expr).str[0] == '&') {
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

  inline result_type help_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr).children;
    
    ASSERT_ARG_COUNT(u, <= 1);

    string s{{"\n", &mem_pool}, true};

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

  inline result_type operator==(unit_ptr const& a, unit_ptr const& b) noexcept {
    if (a.get() == b.get()) {
      SUCCEED_WITH(a->pos, true);
    }

    if (a->expr.index() != b->expr.index()) {
      SUCCEED_WITH(a->pos, true);
    }

    if (is_numeric(a->expr)) {
      SUCCEED_WITH(a->pos, as_numeric(a->expr) == as_numeric(b->expr));
    }

    if (is_string(a->expr)) {
      auto const& sa = as_string(a->expr);
      auto const& sb = as_string(b->expr);
      SUCCEED_WITH(a->pos, sa.raw == sb.raw && sa.str == sb.str);
    }

    if (is_list(a->expr)) {
      auto const& al = as_list(a->expr);
      auto const& bl = as_list(b->expr);

      SUCCEED_WITH(a->pos, al.q == bl.q && al.children == bl.children);
    }
    
    FAIL_WITH("Function comparison is unsupported.", a->pos);
  }

  inline result_type equal_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr).children;
    ASSERT_ARG_COUNT(u, == 2);
    auto ret = args[1] == args[2];
    RETURN_IF_ERROR(ret);
    ret.value()->pos = u->pos;
    return ret;
  }


  inline result_type not_equal_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto ret = equal_m(u, env);
    RETURN_IF_ERROR(ret);
    SUCCEED_WITH(u->pos, !as_numeric(ret.value()->expr));
  }

#define SIMPLE_ORDERING(name, op) \
  inline result_type name##_m(unit_ptr const& u, env_node_ptr& env) noexcept { \
    auto const& args = as_list(u->expr).children; \
    ASSERT_ARG_COUNT(u, == 2); \
    if (args[1]->expr.index() != args[1]->expr.index() \
        || !is_numeric(args[1]->expr)) { \
      FAIL_WITH("Expected two numeric arguments", u->pos);  \
    } \
    SUCCEED_WITH(u->pos, static_cast<numeric>( \
      as_numeric(args[1]->expr) op as_numeric(args[2]->expr) \
    )); \
  }

  SIMPLE_ORDERING(less_than, <);
  SIMPLE_ORDERING(greater_than, >);
  SIMPLE_ORDERING(less_or_equal, <=);
  SIMPLE_ORDERING(greater_or_equal, >=);

  inline result_type if_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr).children;

    ASSERT_ARG_COUNT(u, >= 2);
    ASSERT_ARG_COUNT(u, <= 3);

    bool has_else = args.size() == 4;

    if (!is_numeric(args[1]->expr)) {
      FAIL_WITH("Expected a numeric value.", u->pos);
    }

    if (!as_numeric(args[1]->expr)) {
      if (has_else) {
        return eval(args[3], env, true);
      }
      SUCCEED_WITH(u->pos, list{.q = true});
    } else {
      return eval(args[2], env, true);
    }
  }

  inline result_type readlines_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr).children;
    ASSERT_ARG_COUNT(u, == 1);
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

  inline result_type err_m(unit_ptr const& u, env_node_ptr& env) noexcept {
    auto const& args = as_list(u->expr).children;
    ASSERT_ARG_COUNT(u, == 1);
    if (!is_raw(args[1])) {
      FAIL_WITH(
        "Expected a raw error string",
        args[1]->pos
      );
    }
    FAIL_WITH(as_string(args[1]->expr).str, args[1]->pos);
  }

}
