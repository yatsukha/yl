#include <cctype>
#include <iostream>
#include <fstream>

#include <yl/user_io.hpp>

#include <readline/readline.h>

#ifdef __unix__
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#endif

int main(int const argc, char const* const* argv) {
  auto history_guard = ::yl::make_scope_guard(&::rl_clear_history);

  ::rl_bind_key('\t', [](int, int) { ::rl_insert_text("  "); return 0;  });

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
    
#ifdef __unix__
    auto inotify_descriptor = ::inotify_init();
    ::fcntl(
      inotify_descriptor, 
      F_SETFL, 
      ::fcntl(inotify_descriptor, F_GETFL) | O_NONBLOCK
    );
     auto inotify_descriptor_guard = ::yl::make_scope_guard(
        [inotify_descriptor]{ ::close(inotify_descriptor); }
    );

    auto watch_descriptor = ::inotify_add_watch(
      inotify_descriptor, 
      ".",  
      IN_MODIFY | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVE | IN_MOVE_SELF 
    );
    auto watch_descriptor_guard = ::yl::make_scope_guard(
      [inotify_descriptor, watch_descriptor]{ 
        ::inotify_rm_watch(inotify_descriptor, watch_descriptor); 
      }
    );

    ::std::array<::inotify_event, 16> buffer;

    bool reloading_enabled = true;
#endif

    while (!::yl::handle_line(
      [i = 0, &prompt, &continuation_prompt]() mutable {
        return ::readline((i++ ? continuation_prompt : prompt).c_str());
      }, 
      prompt.size(), 
      ::std::cout,
      ::std::cerr
    )) {
#ifdef __unix__
      if (reloading_enabled && ::read(inotify_descriptor, buffer.data(), sizeof(buffer)) > 0) {
        ::std::cout << "interpreter changed on disk, reload? [y/n]" << "\n";
        char answer;
        ::std::cin >> answer;

        if (::std::tolower(answer) != 'y') {
          continue;
        }

        ::std::cout << "-----------------------------------------" << "\n";

        if (execl(argv[0], argv[0], nullptr) == -1) {
          ::std::cout << "failed to reload, disable reloading? [y/n]"
                      << "\n";
          ::std::cin >> answer;
          if (::std::tolower(answer) == 'y') {
            reloading_enabled = false;
          }
        }
      }
#endif
    }
  }

  ::rl_clear_history();
}
