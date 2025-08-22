#pragma once
#include <vulkan/vulkan.h>

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
        u32 swapchainImageCount;
        VkImageView* swapchainImageViews;

#ifdef DEBUG
        VkDebugReportCallbackEXT debugCallback;
#endif
    };

    extern Graphics graphics;

    void init();
    void deinit();
}
