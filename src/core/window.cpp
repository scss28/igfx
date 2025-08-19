#include "core/window.h"
#include <GLFW/glfw3.h>

namespace igfx::core::window {
    struct {
        GLFWwindow* ptr;
        v2f32 size;
    } window;

    void init() {
        if (!glfwInit()) fatal("failed to initialize glfw");

        window.ptr = glfwCreateWindow(640, 480, "", nullptr, nullptr);
        if (window.ptr == nullptr) {
            deinit();
            fatal("failed to create a window");
        }
    }

    void deinit() {
        glfwTerminate();
    }

    bool shouldClose() {
        glfwSwapBuffers(window.ptr);
        glfwPollEvents();

        return glfwWindowShouldClose(window.ptr);
    }

    v2f32 size() {
        return window.size;
    }
}
