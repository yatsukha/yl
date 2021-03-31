#include <iostream>
#include <fstream>

#include <yl/user_io.hpp>

#include <readline/readline.h>

namespace yl {

  int insert_spaces(int, int) {
    ::rl_insert_text("  ");
    return 0;
  }

}

int main(int const argc, char const* const* argv) {
  ::std::size_t skip_str(char const* line, ::std::size_t pos) noexcept;
  ::rl_bind_key('\t', ::yl::insert_spaces);

  ::std::cout << "yatsukha's lisp" << "\n";
  ::std::cout << "^C to exit, 'help' to get started" << "\n";

  auto const predef = ::yl::load_predef();
  ::std::cout << (predef.empty() ? "loaded predef" : predef) << "\n";

  ::std::ifstream input_file;

  if (argc > 1) {
    input_file = ::std::ifstream{argv[1]};
    if (input_file.is_open()) {
      ::std::cout << "interpreting '" << argv[1] << "'" << "\n\n";
      ::yl::handle_file(::std::move(input_file), ::std::cout, ::std::cerr);
    } else {
      ::std::cerr << "unable to open given file for interpretation: "
                  << argv[1]
                  << "\n"
                  << "exiting"
                  << "\n"; 
    }
  } else {
    ::std::string prompt = "yl> ";
    ::std::string continuation_prompt = "... ";

    while (!::yl::handle_line(
      [i = 0, &prompt, &continuation_prompt]() mutable {
        return ::readline((i++ ? continuation_prompt : prompt).c_str());
      }, 
      prompt.size(), 
      ::std::cout,
      ::std::cerr
    ))
      ;
  }

  ::rl_clear_history();
}
