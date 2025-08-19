#include "igfx/nums.h"
#include "igfx/linalg.h"
#include <print>

// SPDX-FileCopyrightText: 2015 Marek Rusinowski
// SPDX-License-Identifier: MIT
#include <utility>

template <typename F>
class DeferFinalizer {
    F f;
    bool moved;

   public:
    template <typename T>
    DeferFinalizer(T &&f_) : f(std::forward<T>(f_)), moved(false) {}

    DeferFinalizer(const DeferFinalizer &) = delete;

    DeferFinalizer(DeferFinalizer &&other)
        : f(std::move(other.f)), moved(other.moved) {
        other.moved = true;
    }

    ~DeferFinalizer() {
        if (!moved) f();
    }
};

struct Deferrer {
    template <typename F>
    DeferFinalizer<F> operator<<(F &&f) {
        return DeferFinalizer<F>(std::forward<F>(f));
    }
};

extern Deferrer deferrer;

#define TOKENPASTE(x, y) x##y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)
#define defer \
    auto TOKENPASTE2(__deferred_lambda_call, __COUNTER__) = deferrer << [&]

template <class... Args>
_Noreturn void fatal(std::format_string<Args&...> fmt, Args&&... args) {
    std::println(fmt, args...);
    exit(1);
}

