#include <igfx/window.h>
#include <igfx/graphics.h>

using igfx::vec2;

extern "C" void init() {

}

extern "C" void update(f32 deltaTime) {

}

extern "C" void draw(igfx::Frame* frame) {
    igfx::window::size();
}
