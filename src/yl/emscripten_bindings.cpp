/* This file is used for generating a native executable.
 * It is used only in generating WASM for the web frontend. */
#ifdef __EMSCRIPTEN__

#include <sstream>
#include <iostream>
#include <string>

#include <emscripten/bind.h>

#include <yl/user_io.hpp>

namespace yl {

  ::std::string parse_eval(::std::string const str) noexcept {
    ::std::stringstream ss; 
    ::std::cout << str << "\n";
    handle_input(str.c_str(), 0, 0, ss, ss);
    return ss.str();
  }

}

using namespace ::emscripten;

EMSCRIPTEN_BINDINGS(yl) {
  function("parse_eval", &::yl::parse_eval);
}

#endif
