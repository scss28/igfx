#include "igfx/window.h"
#include "core/engine.h"

namespace igfx::window {
    vec2 size() {
        return core::g_Engine.window.size;
    }
}
