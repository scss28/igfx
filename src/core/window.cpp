#include "core/window.h"
#include <GLFW/glfw3.h>

namespace igfx::window {
    Window window;

    void init() {
        if (!glfwInit()) std::fatal("failed to initialize glfw");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        const u32 windowWidth = 640; 
        const u32 windowHeight = 480; 

        GLFWwindow* windowPtr = glfwCreateWindow(
            windowWidth, 
            windowHeight, 
            "", 
            nullptr, 
            nullptr
        );

        if (windowPtr == nullptr) {
            glfwTerminate();
            std::fatal("failed to create a window");
        }

        window = {
            .ptr = windowPtr,
            .width = windowWidth,
            .height = windowHeight,
        };
    }

    void deinit() {
        glfwDestroyWindow(window.ptr);
    }

    u32 width() {
        return window.width;
    }

    u32 height() {
        return window.height;
    }

    vec2 size() {
        return {
            static_cast<f32>(window.width), 
            static_cast<f32>(window.height), 
        };
    }

    bool shouldClose() {
        glfwSwapBuffers(window.ptr);
        glfwPollEvents();

        i32 windowWidth, windowHeight;
        glfwGetWindowSize(window.ptr, &windowWidth, &windowHeight);

        window.width = windowWidth;
        window.height = windowHeight;

        return glfwWindowShouldClose(window.ptr);
    }

    VkSurfaceKHR createSurface(VkInstance instance) {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(
            instance, 
            window.ptr, 
            nullptr, 
            &surface
        ) != VK_SUCCESS) std::fatal("failed to create VkSurfaceKHR");
        return surface;
    }
}
