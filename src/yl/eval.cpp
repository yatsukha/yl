#include <yl/eval.hpp>

#include "builtins.hpp"

namespace yl {

#define BUILTIN(name, desc, bind) \
  {{name, &mem_pool}, make_shared<unit>(unit{{0, 0}, function{{desc, &mem_pool}, bind}})} 
   
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
        "stoi",
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
        def_m
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
      )
    }, 1000, &mem_pool});

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
      
      for (auto& child : ls.children) {
        auto new_child = eval(child, node);
        RETURN_IF_ERROR(new_child);
        child = new_child.value();
      }

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
      return fn.func(make_shared<unit>(pu->pos, ls), node);
    }

    return succeed(pu);
  }

}
