#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace igfx::window {
    struct Window {
        GLFWwindow* ptr;
        u32 width;
        u32 height;
    };

    extern Window window;

    void init();
    void deinit();

    VkSurfaceKHR createSurface(VkInstance);
}
