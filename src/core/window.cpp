#include "core/engine.h"
#include "core/window.h"

#include <GLFW/glfw3.h>

namespace igfx::core::window {
    bool shouldClose() {
        glfwSwapBuffers(g_Engine.window.ptr);
        glfwPollEvents();

        i32 windowWidth, windowHeight;
        glfwGetWindowSize(g_Engine.window.ptr, &windowWidth, &windowHeight);
        g_Engine.window.size = {
            static_cast<f32>(windowWidth), 
            static_cast<f32>(windowHeight),
        };

        return glfwWindowShouldClose(g_Engine.window.ptr);
    }
}
