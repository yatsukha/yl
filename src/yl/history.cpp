#include <yl/history.hpp>

#ifndef __EMSCRIPTEN__
#include <readline/readline.h>
#include <readline/history.h>
#endif

namespace yl {

#ifdef __EMSCRIPTEN__
  seq_representation<string_representation> history::lines;
#endif

  void history::append(char const* line) noexcept {
#ifdef __EMSCRIPTEN__
    lines.push_back(make_string(line));
#else
    ::add_history(line); 
#endif
  }

  char const* history::get(::std::size_t const idx) noexcept {
#ifdef __EMSCRIPTEN__
    return lines[idx].c_str();
#else
    return ::history_list()[idx]->line;
#endif
  }

  ::std::size_t history::size() noexcept {
#ifdef __EMSCRIPTEN__
    return lines.size();
#else
    return ::history_length;
#endif
  }

}
