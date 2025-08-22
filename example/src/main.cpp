#include <igfx/window.h>
#include <igfx/graphics.h>

using igfx::vec2;

extern "C" void init() {

}

extern "C" void update() {

}

extern "C" void draw(igfx::Frame frame) {
    frame.DrawSprite({}, {
        .scale = vec2::one * 20.0f,
    });

    igfx::window::size();
}
