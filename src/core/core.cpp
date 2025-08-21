#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/core.h"
#include "core/engine.h"

namespace igfx::core {
    inline Engine::Window initWindow() {
        if (!glfwInit()) log::fatal("failed to initialize glfw");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        GLFWwindow* windowPtr = glfwCreateWindow(640, 480, "", nullptr, nullptr);
        if (windowPtr == nullptr) {
            glfwTerminate();
            log::fatal("failed to create a window");
        }

        return {
            .ptr = windowPtr,
            .size = vec2::one,
        };
    }

#ifdef _DEBUG
    VkBool32 vkDebugCallback(
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
            log::err("vkError [{}] Code {}: {}", pLayerPrefix, msgCode, pMsg);
    	} else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
            log::warn("vkWarn [{}] Code {}: {}", pLayerPrefix, msgCode, pMsg);
    	}
    
    	return VK_FALSE;
    }
#endif

    VkInstance createVkInstance(u8 const* ppEnabledLayerNames, u32 enableLayerCount) {
        u32 glfwExtensionCount;
        u8 const** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
        u32 requiredExtensionCount = glfwExtensionCount;
#ifdef _DEBUG
        requiredExtensionCount += 1;
#endif
    
        u8 const** requiredExtensions = new u8 const*[requiredExtensionCount];
        defer { delete[] requiredExtensions; };
    
        memcpy(requiredExtensions, glfwExtensions, glfwExtensionCount);
    
#ifdef _DEBUG
        requiredExtensions[requiredExtensionCount - 1] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
#endif
    
        u32 vkExtensionCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, nullptr);
    
        auto vkExtensions = new VkExtensionProperties[vkExtensionCount];
        defer { delete[] vkExtensions; };
    
        vkEnumerateInstanceExtensionProperties(
            nullptr, 
            &requiredExtensionCount, 
            vkExtensions
        );
    
        VkInstanceCreateInfo instanceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .enabledExtensionCount = requiredExtensionCount,
            .ppEnabledExtensionNames = requiredExtensions,
        };
    
#ifdef _DEBUG
        instanceCreateInfo.enabledLayerCount = enableLayerCount;
        instanceCreateInfo.ppEnabledLayerNames = &ppEnabledLayerNames;
#endif
    
        VkInstance instance;
        if (vkCreateInstance(
            &instanceCreateInfo, 
            nullptr, 
            &instance
        ) != VK_SUCCESS) log::fatal("failed to create a VkInstance");

        return instance;
    }

    u32 vkPhysicalDeviceScore(VkPhysicalDevice device) {
        u32 requiredExtensionCount = 1;
        u8 const* requiredExtensions[] {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        u32 extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        VkExtensionProperties* extensions = new VkExtensionProperties[extensionCount];
        for (u32 i = 0; i < requiredExtensionCount; i++) {
            u32 j = 0;
            for (; j < extensionCount; j++) {
                if (strcmp(requiredExtensions[i], extensions[j].extensionName)) break;
            }

            if (j == extensionCount) return 0;
        }

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);

        u32 score = 1 + properties.limits.maxImageDimension2D;
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        return score;
    }

    inline VkPhysicalDevice findVkPhysicalDevice(VkInstance instance) {
        u32 deviceCount;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) log::fatal("failed to find a GPU with vulkan support");

        VkPhysicalDevice* devices = new VkPhysicalDevice[deviceCount];
        defer { delete[] devices; };

        vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

        VkPhysicalDevice selectedDevice = nullptr;
        u32 selectedDeviceScore = 0;

        for(u32 i = 0; i < deviceCount; i++) {
            VkPhysicalDevice device = devices[i];

            u32 score = vkPhysicalDeviceScore(device);
            if (score > selectedDeviceScore) {
                selectedDevice = device;
                selectedDeviceScore = score;
            }
        }

        if (selectedDevice == nullptr) log::fatal("failed to find a suitable GPU");
        return selectedDevice;
    }

    inline void findVkQueueFamilyIndices(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface,
        u32* graphicsQueueFamilyIndex,
        u32* presentQueueFamilyIndex
    ) { 
        u32 queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(
            physicalDevice, 
            &queueFamilyCount, 
            nullptr
        );

        VkQueueFamilyProperties* queueFamilies = 
            new VkQueueFamilyProperties[queueFamilyCount];
        defer { delete[] queueFamilies; };

        vkGetPhysicalDeviceQueueFamilyProperties(
            physicalDevice, 
            &queueFamilyCount, 
            queueFamilies
        );

        *graphicsQueueFamilyIndex = 0xffffffff;
        *presentQueueFamilyIndex = 0xffffffff;

        for (u32 i = 0; i < queueFamilyCount; i++) {
            VkQueueFamilyProperties properties = queueFamilies[i];
            if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                *graphicsQueueFamilyIndex = i;
            }

            VkBool32 presentSupport;
            vkGetPhysicalDeviceSurfaceSupportKHR(
                physicalDevice, 
                i, 
                surface, 
                &presentSupport
            );

            if (presentSupport) {
                *presentQueueFamilyIndex = i;
            }
        }

        if (*graphicsQueueFamilyIndex >= queueFamilyCount) {
            log::fatal("failed to find graphics family queue");
        }

        if (*presentQueueFamilyIndex >= queueFamilyCount) {
            log::fatal("failed to find present family queue");
        }
    }

    inline Engine::Graphics initGraphics(GLFWwindow* window) {
#ifdef _DEBUG
        const u8* ppEnabledLayerNames = "VK_LAYER_LUNARG_standard_validation";
        u32 enableLayerCount = 1;
#else
        const u8* ppEnabledLayerNames = nullptr;
        u32 enableLayerCount = 0;
#endif
        VkInstance instance = createVkInstance(ppEnabledLayerNames, enableLayerCount);
    
#ifdef _DEBUG
        VkDebugReportCallbackCreateInfoEXT debugCreateInfo {
    	    .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
    	    .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
    	    .pfnCallback = (PFN_vkDebugReportCallbackEXT) vkDebugCallback,
        };
    
    	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = 
            (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
            instance, 
            "vkCreateDebugReportCallbackEXT"
        );
    
        VkDebugReportCallbackEXT debugCallback = nullptr;
    	if (CreateDebugReportCallback(
            instance, 
            &debugCreateInfo, 
            nullptr, 
            &debugCallback
        ) != VK_SUCCESS) {
            log::warn("failed to create a debug callback");
    	} 
#endif

        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(
            instance, 
            window, 
            nullptr, 
            &surface
        ) != VK_SUCCESS) log::fatal("failed to create VkSurfaceKHR");

        VkPhysicalDevice physicalDevice = findVkPhysicalDevice(instance);

        u32 graphicsQueueFamilyIndex, presentQueueFamilyIndex;
        findVkQueueFamilyIndices(
            physicalDevice, 
            surface,
            &graphicsQueueFamilyIndex,
            &presentQueueFamilyIndex
        );

        f32 queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfos[] {
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = graphicsQueueFamilyIndex,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority, 
            },
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = presentQueueFamilyIndex,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority, 
            },
        };

        VkPhysicalDeviceFeatures deviceFeatures {};
        VkDeviceCreateInfo deviceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = queueCreateInfos,
            .enabledLayerCount = enableLayerCount,
            .ppEnabledLayerNames = &ppEnabledLayerNames,
            .enabledExtensionCount = 0,
            .pEnabledFeatures = &deviceFeatures,
        };

        VkDevice device;
        if (vkCreateDevice(
            physicalDevice, 
            &deviceCreateInfo, 
            nullptr, 
            &device
        ) != VK_SUCCESS) log::fatal("failed to create VkDevice");

        VkQueue graphicsQueue;
        vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);

        VkQueue presentQueue;
        vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);

        return {
            .instance = instance,
            .device = device,
            .presentQueue = presentQueue,
            .graphicsQueue = graphicsQueue,
            .surface = surface,
#ifdef _DEBUG
            .debugCallback = debugCallback,
#endif
        };
    }

    void init() {
        g_Engine.window = initWindow();
        g_Engine.graphics = initGraphics(g_Engine.window.ptr);
    }

    void deinit() {
        glfwDestroyWindow(g_Engine.window.ptr);

#ifdef _DEBUG
        vkDestroyDebugReportCallbackEXT(
            g_Engine.graphics.instance, 
            g_Engine.graphics.debugCallback, 
            nullptr
        );
#endif

        vkDestroySurfaceKHR(g_Engine.graphics.instance, g_Engine.graphics.surface, nullptr);
        vkDestroyDevice(g_Engine.graphics.device, nullptr);
        vkDestroyInstance(g_Engine.graphics.instance, nullptr);
    }
}
