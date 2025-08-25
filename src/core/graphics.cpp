#include "core/graphics.h"
#include "core/window.h"
#include "igfx/window.h"

#include <std/alloc.h>
#include <std/slice.h>
#include <std/array.h>
#include <std/mem.h>
#include <std/arena.h>
#include <std/math.h>

namespace igfx::graphics {
    Graphics graphics;

    auto requiredDeviceExtensions = std::arr<u8 const*>(
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    );

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
            std::err("[vk] {} {} - '{}'", pLayerPrefix, msgCode, pMsg);
    	} else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
            std::warn("[vk] {} {} - '{}'", pLayerPrefix, msgCode, pMsg);
    	}
    
    	return VK_FALSE;
    }
#endif

    VkInstance createVkInstance(
        std::Slice<u8 const*> ppEnabledLayerNames,
        std::Allocator arena
    ) {
        u32 glfwExtensionCount;
        u8 const** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
        u32 requiredExtensionCount = glfwExtensionCount;
#ifdef DEBUG
        requiredExtensionCount += 1;
#endif
        auto requiredExtensions = arena.alloc<u8 const*>(requiredExtensionCount);
        std::memcpy(
            requiredExtensions[0, glfwExtensionCount],
            std::Slice(glfwExtensions, glfwExtensionCount)
        );
    
#ifdef DEBUG
        requiredExtensions[requiredExtensionCount - 1] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
#endif
    
        u32 vkExtensionCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, nullptr);
    
        auto vkExtensions = arena.alloc<VkExtensionProperties>(vkExtensionCount;
        vkEnumerateInstanceExtensionProperties(
            nullptr, 
            &vkExtensionCount, 
            vkExtensions.ptr
        );

        for (u32 i = 0; i < requiredExtensionCount; i++) {
            u32 j = 0;
            for (; j < vkExtensionCount; j++) {
                if (std::eqlZ(requiredExtensions[i], vkExtensions[j].extensionName)) break;
            }

            if (j == vkExtensionCount) {
                std::err("{} (not found)", requiredExtensions[i]);
            } else {
                std::debug("{} (enabled)", requiredExtensions[i]);
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
            .ppEnabledExtensionNames = requiredExtensions.ptr,
        };

#ifdef DEBUG
        instanceCreateInfo.enabledLayerCount = ppEnabledLayerNames.len;
        instanceCreateInfo.ppEnabledLayerNames = ppEnabledLayerNames.ptr;
#endif

        VkInstance instance;
        VkResult result = vkCreateInstance(
            &instanceCreateInfo, 
            nullptr, 
            &instance
        );
        if (result != VK_SUCCESS) {
            std::fatal("failed to create a VkInstance (errno: {})", (i32)result);
        }

        std::debug("VkInstance created");
        return instance;
    }

    u32 vkPhysicalDeviceScore(VkPhysicalDevice device, std::Allocator arena) {
        u32 extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        auto extensions = arena.alloc<VkExtensionProperties>(extensionCount);
        vkEnumerateDeviceExtensionProperties(
            device, 
            nullptr, 
            &extensionCount, 
            extensions.ptr
        );

        for (u8 const* required : requiredDeviceExtensions) {
            u32 j;
            for (
                j = 0; 
                j < extensionCount && !std::eqlZ(required, extensions[j].extensionName); 
                j++
            );

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

    inline VkPhysicalDevice findVkPhysicalDevice(
        VkInstance instance,
        std::Allocator arena
    ) {
        u32 deviceCount;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) std::fatal("failed to find a GPU with vulkan support");

        auto devices = arena.alloc<VkPhysicalDevice>(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.ptr);

        VkPhysicalDevice selectedDevice = nullptr;
        u32 selectedDeviceScore = 0;

        for(VkPhysicalDevice device : devices) {
            u32 score = vkPhysicalDeviceScore(device, arena);
            if (score > selectedDeviceScore) {
                selectedDevice = device;
                selectedDeviceScore = score;
            }
        }

        if (selectedDevice == nullptr) std::fatal("failed to find a suitable GPU");
        return selectedDevice;
    }

    inline VkSurfaceFormatKHR findVkSurfaceFormat(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface,
        std::Allocator arena
    ) {
        u32 surfaceFormatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(
                physicalDevice, 
                surface, 
                &surfaceFormatCount, 
                nullptr
        );
        if (surfaceFormatCount == 0) std::fatal("no available surface formats");

        auto surfaceFormats = arena.alloc<VkSurfaceFormatKHR>(surfaceFormatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice, 
            surface, 
            &surfaceFormatCount, 
            surfaceFormats.ptr
        );

        for (VkSurfaceFormatKHR format : surfaceFormats) {
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
        VkSurfaceKHR surface,
        std::Allocator arena
    ) {
        u32 presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, 
            surface, 
            &presentModeCount, 
            nullptr
        );
        if (presentModeCount == 0) std::fatal("no available present modes");
        
        auto presentModes = arena.alloc<VkPresentModeKHR>(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, 
            surface, 
            &presentModeCount, 
            presentModes.ptr
        );

        for (VkPresentModeKHR mode : presentModes) {
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
        u32* presentQueueFamilyIndex,
        std::Allocator arena
    ) { 
        u32 queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(
            physicalDevice, 
            &queueFamilyCount, 
            nullptr
        );

        auto queueFamilies = arena.alloc<VkQueueFamilyProperties>(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(
            physicalDevice, 
            &queueFamilyCount, 
            queueFamilies.ptr
        );

        *graphicsQueueFamilyIndex = 0xffffffff;
        *presentQueueFamilyIndex = 0xffffffff;

        for (usize i = 0; i < queueFamilies.len; i++) {
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
            std::fatal("failed to find graphics family queue");
        }

        if (*presentQueueFamilyIndex >= queueFamilyCount) {
            std::fatal("failed to find present family queue");
        }
    }

    void init() {
        std::Arena arena;
        defer { arena.deinit(); };

#ifdef DEBUG
        auto ppEnabledLayerNames = std::arr<u8 const*>(
            "VK_LAYER_KHRONOS_validation"
        );
#else
        std::Array<u8 const*, 0> ppEnabledLayerNames;
#endif
        VkInstance instance = createVkInstance(
            ppEnabledLayerNames.buf(), 
            arena.allocator()
        );
    
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
            std::warn("failed to create a debug callback");
    	} 
#endif

        VkPhysicalDevice physicalDevice = findVkPhysicalDevice(
            instance, 
            arena.allocator()
        );
        VkSurfaceKHR surface = window::createSurface(instance);

        u32 graphicsQueueFamilyIndex, presentQueueFamilyIndex;
        findVkQueueFamilyIndices(
            physicalDevice, 
            surface,
            &graphicsQueueFamilyIndex,
            &presentQueueFamilyIndex,
            arena.allocator()
        );

        f32 queuePriority = 1.0f;
        auto queueCreateInfos = std::arr<VkDeviceQueueCreateInfo>(
            VkDeviceQueueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = graphicsQueueFamilyIndex,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority, 
            },
            VkDeviceQueueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = presentQueueFamilyIndex,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority, 
            }
        );

        VkPhysicalDeviceFeatures deviceFeatures {};
        VkDeviceCreateInfo deviceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = queueCreateInfos.len(),
            .pQueueCreateInfos = queueCreateInfos.data,
            .enabledLayerCount = ppEnabledLayerNames.len(),
            .ppEnabledLayerNames = ppEnabledLayerNames.data,
            .enabledExtensionCount = requiredDeviceExtensions.len(),
            .ppEnabledExtensionNames = requiredDeviceExtensions.data,
            .pEnabledFeatures = &deviceFeatures,
        };

        VkDevice device;
        if (vkCreateDevice(
            physicalDevice, 
            &deviceCreateInfo, 
            nullptr, 
            &device
        ) != VK_SUCCESS) std::fatal("failed to create VkDevice");

        VkQueue graphicsQueue;
        vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);

        VkQueue presentQueue;
        vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);

        VkSurfaceFormatKHR surfaceFormat = findVkSurfaceFormat(
            physicalDevice, 
            surface,
            arena.allocator()
        );
        VkPresentModeKHR presentMode = findVkPresentMode(
            physicalDevice, 
            surface, 
            arena.allocator()
        );

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

        auto queueFamilyIndices = std::arr<u32>( 
            graphicsQueueFamilyIndex,
            presentQueueFamilyIndex
        );

        if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = queueFamilyIndices.len();
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data;
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
            std::fatal("failed to create swapchain (errno: {})", (i32)swapchainResult);
        }

        u32 swapchainImageCount;
        vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);

        auto swapchainImages = arena.allocator().alloc<VkImage>(swapchainImageCount);

        vkGetSwapchainImagesKHR(
            device, 
            swapchain, 
            &swapchainImageCount, 
            swapchainImages.ptr
        );

        auto swapchainImageViews = std::alloc<VkImageView>(swapchainImageCount);
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
            ) != VK_SUCCESS) std::fatal("failed to create image view");
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

        for (VkImageView view : graphics.swapchainImageViews) {
            vkDestroyImageView(graphics.device, view, nullptr);
        }
        std::free(graphics.swapchainImageViews);

        vkDestroySwapchainKHR(graphics.device, graphics.swapchain, nullptr);

        vkDestroySurfaceKHR(graphics.instance, graphics.surface, nullptr);
        vkDestroyDevice(graphics.device, nullptr);
        vkDestroyInstance(graphics.instance, nullptr);
    }
}

