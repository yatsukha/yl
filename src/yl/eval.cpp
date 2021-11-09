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
      BUILTIN_MACRO("'", "Creates a Q expression?", quote_m),
      BUILTIN("eval", "Evaluates a Q expression.", eval_m),
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
      BUILTIN(
        "def",
        "Defines a global variable. 'def {a b} 1 2' assigns 1 and 2 to a and b.",
        def_m
      ),
      BUILTIN(
        "=",
        "Assignes to a local variable. '= {a b} 1 2' assigns 1 and 2 to a and b.",
        assignment_m
      ),
      BUILTIN(
        "decomp",
        "Decomposes a Q expression into local variables.",
        decompose_m
      ),
      BUILTIN(
        "split",
        "Splits a raw string using a delimiter.",
        split_m
      ),
      BUILTIN(
        "\\", 
        "Lambda function, takes a Q expression with symbols as arguments "
        "and a Q expression as a body to evaluate. Returns a callable function.\n"
        "For example '(\\{x y} {+ x y}) 2 3' will yield 5.\n"
        "It can also take a docstring: '\\{x y} \"add\" {+ x y}'.",
        lambda_m
      ),
      BUILTIN("help", "Outputs information about a symbol.", help_m),
      BUILTIN("==", "Compares arguments for equality.", equal_m),
      BUILTIN("!=", "Compares arguments for inequality.", not_equal_m),
      BUILTIN("<", "Tests if first number is less than second.", less_than_m),
      BUILTIN(">", "Tests if first number is greater than second.", greater_than_m),
      BUILTIN("<=", "Tests if first number is less than or equal to second.", less_or_equal_m),
      BUILTIN(">=", "Tests if first number is greater than or equal to second.", greater_or_equal_m),
      BUILTIN(
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
        "mk_map",
        "Creates a map from pairs, example input: {\"1\" 1 \"2\" 2}.",
        mk_map_m
      ),
      BUILTIN(
        "is_atom",
        "Check whether the expression inside the Q expression is an atom.\n"
        "is_atom {aa} => 1\n"
        "is_atom {}   => 0\n"
        "is_atom {{}} => 0\n"
        "is_atom {()} => 1",
        is_atom_m
      ),
      BUILTIN(
        "collapse",
        "Collapses an nested list inside Q expression.\n"
        "Also useful for dealing with non Q lists inside Q expressions.\n"
        "collapse {{}} => {}\n"
        "collapse {()} => {}\n"
        "collapse {(doesn't need to be empty)} => {doesn't need to be empty}",
        collapse_m
      ),
      BUILTIN(
        "is_list",
        "Checks whether the expression yields the specified type.",
        is_list_m
      ),
      BUILTIN(
        "is_numeric",
        "Checks whether the expression yields the specified type.",
        is_numeric_m
      ),
      BUILTIN(
        "is_hash_map",
        "Checks whether the expression yields the specified type.",
        is_hash_map_m
      ),
      BUILTIN(
        "is_function",
        "Checks whether the expression yields the specified type.",
        is_function_m
      ),
      BUILTIN(
        "is_q",
        "Checks whether the expression yields the specified type.",
        is_q_m
      ),
      BUILTIN(
        "is_raw",
        "Checks whether the expression yields the specified type.",
        is_raw_m
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

    auto starting_point = node;
    auto s = as_string(pu->expr).str;

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
    if (is_string(pu->expr)) {
      if (as_string(pu->expr).raw) {
        return succeed(pu);
      }

      return resolve_symbol(pu, node);
    }

    if (is_list(pu->expr)) {
      auto ls = as_list(pu->expr);

      if (ls.children.empty() || (!force_eval && ls.q)) {
        return succeed(pu);
      }

      auto const front = eval(ls.children[0], node);
      RETURN_IF_ERROR(front);
      ls.children[0] = front.value();

      auto const is_fn = is_function(ls.children.front()->expr);

      if (ls.children.size() == 1 && !is_fn) {
        return succeed(ls.children.front());
      }

      if (!is_fn) {
        FAIL_WITH(
          "Expected a builtin or user defined function.",
          ls.children.front()->pos
        );
      }

      auto const& fn = as_function(ls.children.front()->expr);

      if (!fn.macro) {
        ::std::cout << fn.description << " >> is not a macro? " << "\n";
        for (::std::size_t i = 1; i < ls.children.size(); ++i) {
          auto& child = ls.children[i];
          auto new_child = eval(child, node);
          RETURN_IF_ERROR(new_child);
          child = new_child.value();
        }
      }

      return fn.func(make_shared<unit>(pu->pos, ls), node);
    }

    return succeed(pu);
  }

}
