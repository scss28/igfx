#include <vulkan/vulkan.h>
#include <print>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef _DEBUG
VkBool32 debugCallback(
        VkDebugReportFlagsEXT flags, 
        VkDebugReportObjectTypeEXT objType, 
        u64 srcObject, 
        usize location, 
        i32 msgCode, 
        const u8* pLayerPrefix, 
        const u8* pMsg, 
        void* pUserData
) {
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        std::println("VK_ERROR: [{}] Code {}: {}", pLayerPrefix, msgCode, pMsg);
	} else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        std::println("VK_WARNING: [{}] Code {}: {}", pLayerPrefix, msgCode, pMsg);
	}

	return VK_FALSE;
}
#endif

struct {
    VkInstance instance;

#ifdef _DEBUG
    VkDebugReportCallbackEXT debugCallback;
#endif
} graphics;

void init() {
    u32 glfwExtensionCount;
    const u8** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    auto requiredExtensionCount = glfwExtensionCount;
#ifdef _DEBUG
    requiredExtensionCount += 1;
#endif

    auto requiredExtensions = new u8 const*[requiredExtensionCount];
    defer { delete[] requiredExtensions; };

    memcpy(requiredExtensions, glfwExtensions, glfwExtensionCount);

#ifdef _DEBUG
    requiredExtensions[requiredExtensionCount - 1] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
#endif

    // u32 vkExtensionCount;
    // vkEnumerateInstanceExtensionProperties(nullptr, &requiredExtensionCount, nullptr);

    // auto vkExtensions = new VkExtensionProperties[vkExtensionCount];
    // defer { delete[] vkExtensions; };

    // vkEnumerateInstanceExtensionProperties(nullptr, &requiredExtensionCount, vkExtensions);

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledExtensionCount = requiredExtensionCount,
        .ppEnabledExtensionNames = requiredExtensions,
    };

#ifdef _DEBUG
    const u8* ppEnabledLayerNames = "VK_LAYER_LUNARG_standard_validation";
    instanceCreateInfo.enabledLayerCount = 1;
    instanceCreateInfo.ppEnabledLayerNames = &ppEnabledLayerNames;
#endif

    if (vkCreateInstance(&instanceCreateInfo, nullptr, &graphics.instance) != VK_SUCCESS) {
        fatal("failed to create a Vulkan instance");
    }

#ifdef _DEBUG
    VkDebugReportCallbackCreateInfoEXT debugCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
	    .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
	    .pfnCallback = (PFN_vkDebugReportCallbackEXT) debugCallback,
    };

	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = 
        (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(graphics.instance, "vkCreateDebugReportCallbackEXT");

	if (CreateDebugReportCallback(
        graphics.instance, 
        &debugCreateInfo, 
        nullptr, 
        &graphics.debugCallback
    ) != VK_SUCCESS) {
        println("failed to create debug callback");
	} 
#endif

    GLFWwindow* window;
    VkSurfaceKHR windowSurface;
    if (glfwCreateWindowSurface(graphics.instance, window, NULL, &windowSurface) != VK_SUCCESS) {
        exit(1);
    }

}

