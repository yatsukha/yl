#include <iostream>
#include <fstream>

#include <yl/user_io.hpp>

#include <readline/readline.h>

namespace yl {

  int insert_spaces(int, int) {
    ::rl_insert_text("  ");
    return 0;
  }

  auto handle_file(
    ::std::ifstream&& file,
    ::std::ostream& out,
    ::std::ostream& err
  ) noexcept {
    while (!::yl::handle_line(
      [&file] { 
        ::std::size_t constexpr static buf_size = 1 << 13;
        char* buf = reinterpret_cast<char*>(::malloc(buf_size));

        if (file.getline(buf, buf_size)) {
          return buf;
        }
        
        ::free(buf);
        return static_cast<char*>(nullptr);
      }, 
      0, out, err
    ))
      ;
  }

}

int main(int const argc, char const* const* argv) {
  ::rl_bind_key('\t', ::yl::insert_spaces);

  ::std::cout << "yatsukha's lisp" << "\n";
  ::std::cout << "^C to exit, 'help' to get started" << "\n";

  ::std::string predef_name = ".predef.yl";
  ::std::ifstream predef(predef_name);

  if (predef.is_open()) {
    ::std::ostream dev_null{nullptr};

    ::std::cout << "detected predef at '" << predef_name
                << "', reading..."
                << "\n";

    ::yl::handle_file(::std::move(predef), dev_null, ::std::cerr);
  }

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
