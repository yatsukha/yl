/* This file is NOT used for generating a native executable.
 * It is used only for generating WASM for the web frontend. */
#ifdef __EMSCRIPTEN__

#include <sstream>
#include <iostream>
#include <string>

#include <emscripten/bind.h>

#include <yl/user_io.hpp>
#include <yl/history.hpp>
#include <yl/parse.hpp>

namespace yl {

  ::std::string parse_eval(::std::string const str,
                           bool const continuated = false) noexcept {
    ::std::stringstream ss; 
    handle_input(str.c_str(), 0, continuated, ss, ss);
    return ss.str();
  }

  int paren_balance_wrapper(::std::string const str) noexcept {
    return paren_balance(str.c_str());
  }

}

using namespace ::emscripten;

EMSCRIPTEN_BINDINGS(yl) {
  function("parse_eval", &::yl::parse_eval);

  class_<::yl::history>("history")
    .class_function("append", &::yl::history::append)
    .class_function("get", &::yl::history::get)
    .class_function("size", &::yl::history::size);

  function("paren_balance", &::yl::paren_balance_wrapper);
  function("load_predef", &::yl::load_predef);
}

#endif
