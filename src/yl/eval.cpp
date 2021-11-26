#include <iostream>
#include <yl/eval.hpp>

#include "builtins.hpp"
#include "yl/types.hpp"

namespace yl {

#define BUILTIN_MACRO(name, desc, bind) \
  { \
    make_string(name), \
    make_shared<unit>(unit{{0, 0}, function{make_string(desc), bind, true}})} 

#define BUILTIN(name, desc, bind) \
  { \
    make_string(name), \
    make_shared<unit>(unit{{0, 0}, function{make_string(desc), bind}})} 
   
  env_node_ptr global_environment() noexcept {
    auto static g_env = make_shared(environment{{
      BUILTIN(
        ",",
        "Forces evaluation of an argument to macro function.\n"
        "(\\m (x) (x)) + => + (symbol)\n"
        "(\\m (x) (q , x)) , + => <<+ function>>\n"
        "NOTE: it is not recursive! "
        "You can not evaluate an expression inside another unevaluated one.",
        keyword_m
      ),
      BUILTIN("+", "Adds numbers.", add_m),
      BUILTIN("-", "Subtracts numbers.", sub_m),
      BUILTIN("*", "Multiplies numbers.", mul_m),
      BUILTIN("/", "Divides numbers.", div_m),
      BUILTIN("%", "Modulo.", mod_m),
      BUILTIN("&", "Binary and.", and_m),
      BUILTIN("|", "Binary or.", or_m),
      BUILTIN("^", "Binary xor.", xor_m),
      BUILTIN("<<", "Shift left.", shl_m),
      BUILTIN(">>", "Shift right.", shr_m),
      BUILTIN_MACRO("q",     "Creates a Q expression?", quote_m),
      BUILTIN_MACRO("quote", "Creates a Q expression?", quote_m),
      BUILTIN("e",    "Evaluates a Q expression.", eval_m),
      BUILTIN("eval", "Evaluates a Q expression.", eval_m),
      BUILTIN("echo", "Echoes the value.", echo_m),
      BUILTIN("list", "Takes arguments and turns them into a Q expression.", list_m),
      BUILTIN("head", "Returns the first element of a list or a string.", head_m),
      BUILTIN("tail", "Returns the list/string without it's first element.", tail_m),
      BUILTIN("last", "Returns the last element of a list/string.", last_m),
      BUILTIN("join", "Joins one or more Q expressions or raw strings.", join_m),
      BUILTIN("cons", "Appends its first argument to the second Q expression.", cons_m),
      BUILTIN("at", "Indexes into a Q expression or  a raw string.", at_m),
      BUILTIN("len", "Calculates the length of a Q expression or a raw string.", len_m),
      BUILTIN("init", "Returns a Q expression or a raw string without it's last element.", init_m),
      BUILTIN(
        "sorted",
        "Returns a new Q expression with sorted elements. Supports custom comparator.",
        sorted_m
      ),
      BUILTIN(
        "int",
        "Converts a raw string to an integer.",
        stoi_m
      ),
      BUILTIN_MACRO(
        "def",
        "Defines a global variable. 'def {a b} 1 2' assigns 1 and 2 to a and b.",
        def_m
      ),
      BUILTIN_MACRO(
        "=",
        "Assignes to a local variable. '= {a b} 1 2' assigns 1 and 2 to a and b.",
        assignment_m
      ),
      BUILTIN_MACRO(
        "decomp",
        "Decomposes a Q expression into local variables.",
        decompose_m
      ),
      BUILTIN(
        "split",
        "Splits a raw string using a delimiter.",
        split_m
      ),
      BUILTIN_MACRO(
        "\\", 
        "Lambda function, takes a Q expression with symbols as arguments "
        "and a Q expression as a body to evaluate. Returns a callable function.\n"
        "For example '(\\{x y} {+ x y}) 2 3' will yield 5.\n"
        "It can also take a docstring: '\\{x y} \"add\" {+ x y}'.",
        lambda_m
      ),
      BUILTIN_MACRO(
        "\\m", 
        "Macro function. Same as \\ but does not evaluate arguments.",
        macro_m
      ),
      BUILTIN_MACRO(
        "\\s", 
        "Syntax macro. The same as regular macro \\m except for how closure is defined.\n"
        "Symbols are captured from the place where the function is called, rather\n"
        "than where the function is created. This distinction is important for\n"
        "creating new syntax for example an 'or' function that can properly evaluate\n"
        "it's arguments.",
        syntax_m
      ),
      BUILTIN("help", "Outputs information about a symbol.", help_m),
      BUILTIN("==", "Compares arguments for equality.", equal_m),
      BUILTIN_MACRO("is_equal", "Compares unevaluated arguments for equality.", equal_m),
      BUILTIN("!=", "Compares arguments for inequality.", not_equal_m),
      BUILTIN("<", "Tests if first number is less than second.", less_than_m),
      BUILTIN(">", "Tests if first number is greater than second.", greater_than_m),
      BUILTIN("<=", "Tests if first number is less than or equal to second.", less_or_equal_m),
      BUILTIN(">=", "Tests if first number is greater than or equal to second.", greater_or_equal_m),
      BUILTIN_MACRO(
        "if", 
        "Evaluates a Q expression depending on the condition.\n"
        "Example: 'if (== 1 2) { 1 } { 2 }' will yield 2.\n"
        "Using Q expression, {}, is optional if the bodies do "
        "not need to be lazily evaluated: 'if (== 1 1) 0 1'.\n"
        "You can also omit the second body if you do not need an else:\n"
        "'if (< x 0) { def {x} (+ x 1) }'",
        if_m
      ),
      BUILTIN(
        "readlines", 
        "Yields a Q expression that contains lines from the file.\n"
        "Example: 'readlines \"data.txt\"",
        readlines_m
      ),
      BUILTIN(
        "err",
        "Terminates interpretation of the current line.\n"
        "Example: err \"Argument must be greater than 0.\".",
        err_m
      ),
      BUILTIN(
        "str",
        "Converts an expression to string.",
        str_m
      ),
      BUILTIN(
        "mk-map",
        "Creates a map from pairs, example input: (q (\"1\" 1 \"2\" 2).",
        mk_map_m
      ),
      BUILTIN(
        "atom?",
        "Check whether the expression is an atom (not a collection).",
        is_atom_m
      ),
      BUILTIN_MACRO(
        "__while",
        "Used exclusively for library optimization. Do not use in regular code.\n"
        "This is required to allow users to write trampolining tail recursion\n"
        "optimization, since you need a procedural while to achieve it.",
        while_m
      ),
      BUILTIN(
        "list?",
        "Checks whether the expression yields the specified type.",
        is_list_m
      ),
      BUILTIN(
        "numeric?",
        "Checks whether the expression yields the specified type.",
        is_numeric_m
      ),
      BUILTIN(
        "map?",
        "Checks whether the expression yields the specified type.",
        is_hash_map_m
      ),
      BUILTIN(
        "function?",
        "Checks whether the expression yields the specified type.",
        is_function_m
      ),
      BUILTIN(
        "raw?",
        "Checks whether the expression yields the specified type.",
        is_raw_m
      ),
      BUILTIN(
        "time-ms",
        "Retrieves the time since epoch in milliseconds.",
        time_ms_m
      ),
      BUILTIN(
        "null?",
        "Checks whether the value is ().",
        is_null_m
      ),
    }, 1000
#ifndef __EMSCRIPTEN__
    , &mem_pool
#endif
  });

    return make_shared<env_node>(env_node{
      .curr = g_env,
      .prev = {}
    });
  }

  result_type resolve_symbol(unit_ptr const& pu, env_node_ptr node) noexcept {
    if (!is_string(pu->expr)) {
      FAIL_WITH("Expected a symbol.", pu->pos);
    }

    //::std::cout << pu->expr << "\n";

    auto starting_point = node;
    auto s = as_string(pu->expr).str;

    auto iter = node->curr->begin();
    while ((iter = node->curr->find(s)) == node->curr->end()) {
      if (!node->prev) {
        FAIL_WITH(concat("Symbol ", s, " is undefined."), pu->pos); 
      }
      node = node->prev;
    }

    auto next = iter->second;

    return succeed(make_shared(unit{pu->pos, next->expr}));
  }

  result_type eval(
    unit_ptr const& pu, 
    env_node_ptr node
  ) noexcept {
    if (is_string(pu->expr)) {
      if (as_string(pu->expr).raw) {
        return succeed(pu);
      }

      return resolve_symbol(pu, node);
    }

    if (is_list(pu->expr)) {
      auto ls = as_list(pu->expr);

      if (ls.empty()) {
        return succeed(pu);
      }

      auto const front = eval(ls[0], node);
      RETURN_IF_ERROR(front);
      ls[0] = front.value();

      auto const is_fn = is_function(ls.front()->expr);

      if (ls.size() == 1 && !is_fn) {
        return succeed(ls.front());
      }

      if (!is_fn) {
        FAIL_WITH(
          concat(
            "Expected a builtin or user defined function, got ",
            type_of(ls.front()->expr),
            " with value ",
            ls.front()->expr,
            ", complete expression: ",
            pu->expr,
            "."
          ),
          ls.front()->pos
        );
      }

      auto const& fn = as_function(ls.front()->expr);
      if (!fn.macro) {
        for (::std::size_t i = 1; i < ls.size(); ++i) {
          auto& child = ls[i];
          auto new_child = eval(child, node);
          RETURN_IF_ERROR(new_child);
          child = new_child.value();
        }
      } else {
        ::std::size_t shrink = 0;
        auto move_forward = [&shrink, &ls](auto&& i) {
          if (shrink) {
            ls[i - shrink] = ls[i];
          }
        };

        for (::std::size_t i = 1; i < ls.size(); ++i) {
          auto& child = ls[i]->expr;
          if (!is_string(child)) {
            move_forward(i);
            continue;
          }
          auto& sym = as_string(child);
          if (sym.raw) {
            move_forward(i);
            continue;
          }

          if (sym.str == ",") {
            auto const v = eval(ls[i + 1], node);
            RETURN_IF_ERROR(v);
            ls[i - shrink] = v.value();
            ++shrink;
            i += 1;
          } else {
            move_forward(i);
          }
        }

        ls.resize(ls.size() - shrink);
      }

      return fn.func(::yl::make_shared<unit>(pu->pos, ls), node);
    }

    return succeed(pu);
  }

}
