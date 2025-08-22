#include "core/graphics.h"
#include "core/window.h"

namespace igfx::engine {
    void init() {
        window::init();
        graphics::init();
    }

    void deinit() {
        window::deinit();
        graphics::deinit();
    }
}
