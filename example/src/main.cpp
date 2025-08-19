#include <print>

#include <igfx/window.h>
#include <igfx/graphics.h>

extern "C" void init() {

}

extern "C" void update() {

}

extern "C" void draw(igfx::Frame* frame) {
    igfx::windowSize();
}
