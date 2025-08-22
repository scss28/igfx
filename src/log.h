#pragma once
#include <print>

namespace igfx::log {
    template <class... Args>
    void debug(std::format_string<Args&...> fmt, Args&&... args) {
#ifdef DEBUG
        std::print("\033[90mdebug:\033[0m ");
        std::println(fmt, args...);
#endif
    }

    template <class... Args>
    void info(std::format_string<Args&...> fmt, Args&&... args) {
        std::print("\033[32minfo:\033[0m ");
        std::println(fmt, args...);
    }

    template <class... Args>
    void warn(std::format_string<Args&...> fmt, Args&&... args) {
        std::print("\033[91mwarn:\033[0m ");
        std::println(fmt, args...);
    }
    
    template <class... Args>
    void err(std::format_string<Args&...> fmt, Args&&... args) {
        std::print("\033[31merror:\033[0m ");
        std::println(fmt, args...);
    }

    template <class... Args>
    _Noreturn void fatal(std::format_string<Args&...> fmt, Args&&... args) {
        err(fmt, args...);
        exit(1);
    }
}
