#pragma once
#include <vulkan/vulkan.h>
#include <std/slice.h>

namespace igfx::graphics {
    struct Graphics {
        VkInstance instance;
        VkDevice device;
        VkQueue presentQueue;
        VkQueue graphicsQueue;
        VkSurfaceKHR surface;

        VkFormat swapchainImageFormat;
        VkExtent2D swapchainExtent;
        VkSwapchainKHR swapchain;
        std::Buf<VkImageView> swapchainImageViews;

#ifdef DEBUG
        VkDebugReportCallbackEXT debugCallback;
#endif
    };

    extern Graphics graphics;

    void init();
    void deinit();
}
