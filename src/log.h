#pragma once
#include <print>

namespace igfx::log {
    template <class... Args>
    void info(std::format_string<Args&...> fmt, Args&&... args) {
        std::print("info: ");
        std::println(fmt, args...);
    }

    template <class... Args>
    void warn(std::format_string<Args&...> fmt, Args&&... args) {
        std::print("warn: ");
        std::println(fmt, args...);
    }
    
    template <class... Args>
    void err(std::format_string<Args&...> fmt, Args&&... args) {
        std::print("err: ");
        std::println(fmt, args...);
    }

    template <class... Args>
    _Noreturn void fatal(std::format_string<Args&...> fmt, Args&&... args) {
        err(fmt, args...);
        exit(1);
    }
}
