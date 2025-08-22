#include "core/graphics.h"
#include "core/window.h"
#include "igfx/window.h"

#include <array>

namespace igfx::graphics {
    Graphics graphics;

    std::array<u8 const*, 1> requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

#ifdef DEBUG
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
            log::err("[vk] {} {} - '{}'", pLayerPrefix, msgCode, pMsg);
    	} else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
            log::warn("[vk] {} {} - '{}'", pLayerPrefix, msgCode, pMsg);
    	}
    
    	return VK_FALSE;
    }
#endif

    VkInstance createVkInstance(u8 const* ppEnabledLayerNames, u32 enableLayerCount) {
        u32 glfwExtensionCount;
        u8 const** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
        u32 requiredExtensionCount = glfwExtensionCount;
#ifdef DEBUG
        requiredExtensionCount += 1;
#endif
        u8 const** requiredExtensions = new u8 const*[requiredExtensionCount];
        defer { delete[] requiredExtensions; };
    
        memcpy(requiredExtensions, glfwExtensions, glfwExtensionCount * sizeof(u8 const*));
    
#ifdef DEBUG
        requiredExtensions[requiredExtensionCount - 1] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
#endif
    
        u32 vkExtensionCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, nullptr);
    
        auto vkExtensions = new VkExtensionProperties[vkExtensionCount];
        defer { delete[] vkExtensions; };
    
        vkEnumerateInstanceExtensionProperties(
            nullptr, 
            &vkExtensionCount, 
            vkExtensions
        );

        for (u32 i = 0; i < requiredExtensionCount; i++) {
            u32 j = 0;
            for (; j < vkExtensionCount; j++) {
                if (strcmp(requiredExtensions[i], vkExtensions[j].extensionName)) break;
            }

            if (j == vkExtensionCount) {
                log::err("{} (not found)", requiredExtensions[i]);
            } else {
                log::debug("{} (enabled)", requiredExtensions[i]);
            }
        }

        VkApplicationInfo applicationInfo {
		    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		    .pApplicationName = "igfx app",
		    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		    .pEngineName = "igfx",
		    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
		    .apiVersion = VK_API_VERSION_1_0,
        };
    
        VkInstanceCreateInfo instanceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = requiredExtensionCount,
            .ppEnabledExtensionNames = requiredExtensions,
        };

#ifdef DEBUG
        instanceCreateInfo.enabledLayerCount = enableLayerCount;
        instanceCreateInfo.ppEnabledLayerNames = &ppEnabledLayerNames;
#endif

        VkInstance instance;
        VkResult result = vkCreateInstance(
            &instanceCreateInfo, 
            nullptr, 
            &instance
        );
        if (result != VK_SUCCESS) {
            log::fatal("failed to create a VkInstance (errno: {})", (i32)result);
        }

        log::debug("VkInstance created");
        return instance;
    }

    u32 vkPhysicalDeviceScore(VkPhysicalDevice device) {
        u32 extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        VkExtensionProperties* extensions = new VkExtensionProperties[extensionCount];
        for (u32 i = 0; i < requiredDeviceExtensions.size(); i++) {
            u32 j = 0;
            for (; j < extensionCount; j++) {
                if (strcmp(requiredDeviceExtensions[i], extensions[j].extensionName)) break;
            }

            if (j == extensionCount) return 0; // extension not found...
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

    inline VkSurfaceFormatKHR findVkSurfaceFormat(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface
    ) {
        u32 surfaceFormatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(
                physicalDevice, 
                surface, 
                &surfaceFormatCount, 
                nullptr
        );
        if (surfaceFormatCount == 0) log::fatal("no available surface formats");

        VkSurfaceFormatKHR* surfaceFormats = new VkSurfaceFormatKHR[surfaceFormatCount];
        defer { delete[] surfaceFormats; };

        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice, 
            surface, 
            &surfaceFormatCount, 
            surfaceFormats
        );

        for (u32 i = 0; i < surfaceFormatCount; i++) {
            VkSurfaceFormatKHR format = surfaceFormats[0];
            if (
                format.format == VK_FORMAT_B8G8R8A8_SRGB 
                && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            ) {
                return format;
            }
        }

        return surfaceFormats[0];
    }

    inline VkPresentModeKHR findVkPresentMode(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface
    ) {
        u32 presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice, 
                surface, 
                &presentModeCount, 
                nullptr
        );
        if (presentModeCount == 0) log::fatal("no available present modes");
        
        VkPresentModeKHR* presentModes = new VkPresentModeKHR[presentModeCount];
        defer { delete[] presentModes; };

        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, 
            surface, 
            &presentModeCount, 
            presentModes
        );

        for (u32 i = 0; i < presentModeCount; i++) {
            VkPresentModeKHR mode = presentModes[i];
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    inline VkExtent2D findVkExtent2D(VkSurfaceCapabilitiesKHR surfaceCapabilities) {
        u32 minWidth = surfaceCapabilities.minImageExtent.width;
        u32 maxWidth = surfaceCapabilities.maxImageExtent.width;

        u32 minHeight = surfaceCapabilities.minImageExtent.height;
        u32 maxHeight = surfaceCapabilities.maxImageExtent.height;

        return {
            std::clamp(window::width(), minWidth, maxWidth),
            std::clamp(window::height(), minHeight, maxHeight),
        };
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

    void init() {
#ifdef DEBUG
        const u8* ppEnabledLayerNames = "VK_LAYER_KHRONOS_validation";
        u32 enableLayerCount = 1;
#else
        const u8* ppEnabledLayerNames = nullptr;
        u32 enableLayerCount = 0;
#endif
        VkInstance instance = createVkInstance(ppEnabledLayerNames, enableLayerCount);
    
#ifdef DEBUG
        VkDebugReportCallbackCreateInfoEXT debugCreateInfo {
    	    .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
    	    .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
    	    .pfnCallback = (PFN_vkDebugReportCallbackEXT) vkDebugCallback,
        };
    
    	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallback = 
            (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
            instance, 
            "vkCreateDebugReportCallbackEXT"
        );
    
        VkDebugReportCallbackEXT debugCallback;
    	if (vkCreateDebugReportCallback(
            instance, 
            &debugCreateInfo, 
            nullptr, 
            &debugCallback
        ) != VK_SUCCESS) {
            log::warn("failed to create a debug callback");
    	} 
#endif

        VkPhysicalDevice physicalDevice = findVkPhysicalDevice(instance);
        VkSurfaceKHR surface = window::createSurface(instance);

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
            .queueCreateInfoCount = 2,
            .pQueueCreateInfos = queueCreateInfos,
            .enabledLayerCount = enableLayerCount,
            .ppEnabledLayerNames = &ppEnabledLayerNames,
            .enabledExtensionCount = requiredDeviceExtensions.size(),
            .ppEnabledExtensionNames = requiredDeviceExtensions.data(),
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

        VkSurfaceFormatKHR surfaceFormat = findVkSurfaceFormat(physicalDevice, surface);
        VkPresentModeKHR presentMode = findVkPresentMode(physicalDevice, surface);

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            physicalDevice,
            surface,
            &surfaceCapabilities
        );

        VkExtent2D swapchainExtent = findVkExtent2D(surfaceCapabilities);

        VkSwapchainCreateInfoKHR swapchainCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface,
            .minImageCount = surfaceCapabilities.minImageCount + 1,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = swapchainExtent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .preTransform = surfaceCapabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = nullptr,
        };

        u32 queueFamilyIndices[] = {
            graphicsQueueFamilyIndex,
            presentQueueFamilyIndex,
        };

        if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = 2;
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        VkSwapchainKHR swapchain;
        VkResult swapchainResult = vkCreateSwapchainKHR(
            device, 
            &swapchainCreateInfo, 
            nullptr, 
            &swapchain
        );

        if (swapchainResult != VK_SUCCESS) {
            log::fatal("failed to create swapchain (errno: {})", (i32)swapchainResult);
        }

        u32 swapchainImageCount;
        vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);

        VkImage* swapchainImages = new VkImage[swapchainImageCount];
        defer { delete[] swapchainImages; };
        vkGetSwapchainImagesKHR(
            device, 
            swapchain, 
            &swapchainImageCount, 
            swapchainImages
        );

        VkImageView* swapchainImageViews = new VkImageView[swapchainImageCount];
        for (u32 i = 0; i < swapchainImageCount; i++) {
            VkImageViewCreateInfo imageViewCreateInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = swapchainImages[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = surfaceFormat.format,
                .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

            if (vkCreateImageView(
                device, 
                &imageViewCreateInfo, 
                nullptr, 
                &swapchainImageViews[i]
            ) != VK_SUCCESS) log::fatal("failed to create image view");
        }

        graphics = {
            .instance = instance,
            .device = device,
            .presentQueue = presentQueue,
            .graphicsQueue = graphicsQueue,
            .surface = surface,

            .swapchainImageFormat = surfaceFormat.format,
            .swapchainExtent = swapchainExtent,
            .swapchain = swapchain,
            .swapchainImageCount = swapchainImageCount,
            .swapchainImageViews = swapchainImageViews,
#ifdef DEBUG
            .debugCallback = debugCallback,
#endif
        };
    }

    void deinit() {
#ifdef DEBUG
    	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugCallback = 
            (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
            graphics.instance,
            "vkDestroyDebugReportCallbackEXT"
        );

        vkDestroyDebugCallback(
            graphics.instance, 
            graphics.debugCallback, 
            nullptr
        );
#endif

        for (u32 i = 0; i < graphics.swapchainImageCount; i++) {
            vkDestroyImageView(graphics.device, graphics.swapchainImageViews[i], nullptr);
        }
        delete[] graphics.swapchainImageViews;

        vkDestroySwapchainKHR(graphics.device, graphics.swapchain, nullptr);

        vkDestroySurfaceKHR(graphics.instance, graphics.surface, nullptr);
        vkDestroyDevice(graphics.device, nullptr);
        vkDestroyInstance(graphics.instance, nullptr);
    }
}

