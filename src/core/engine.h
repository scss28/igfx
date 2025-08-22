#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace igfx::core {
    struct Engine {
        struct Window {
            GLFWwindow* ptr;
            vec2 size;
        } window;
        struct Graphics {
            VkInstance instance;
            VkDevice device;
            VkQueue presentQueue;
            VkQueue graphicsQueue;
            VkSurfaceKHR surface;

#ifdef DEBUG
            VkDebugReportCallbackEXT debugCallback;
#endif
        } graphics;
    };
    
    extern Engine g_Engine;
}
