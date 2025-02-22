#include "vulkan.h"

#include "core/list_types.h"
#include "core/filesystem.h"
#include "core/window.h"
#include "core/camera.h"
#include "core/timer.h"
#include "core/core.h"
#include "core/list.h"
#include "core/vec.h"

#include "vulkan_base.h"
#include "raytracing.h"
#include "texture.h"
#include "buffer.h"
#include "shader.h"
#include "image.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

extern VkPhysicalDevice physicalDevice;
extern VkDevice device;

extern VkQueue graphicsQueue, presentQueue;

typedef struct {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
} QueueFamilyIndices;

typedef struct {
    Vec4 position;
    Vec4 color;
    Vec2 texCoord;

    Vec2 padding;
} Vertex;

LIST_DEFINE(const char*, Extensions);
LIST_DEFINE(VkSurfaceFormatKHR, SurfaceFormatKHRs);
LIST_DEFINE(VkPresentModeKHR, PresentModeKHRs);

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    SurfaceFormatKHRs        formats;
    PresentModeKHRs          presentModes;
} SwapChainSupportDetails;

const char* validationLayers[] = {
    "VK_LAYER_KHRONOS_validation",
};

const char* deviceExensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME,

    VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
    
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_DEVICE_GROUP_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    VK_KHR_RAY_QUERY_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME,
    VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
    VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
};

#define MAX_FRAMES_IN_FLIGHT 2

const Vertex vertices[] = {
    (Vertex) {
        .position = { -0.5f, -0.5f, 0.0f, 1.0f },
        .color    = { 1.0f, 0.0f, 0.0f, 1.0f },
        .texCoord = { 1.0f, 0.0f },
    },
    (Vertex) {
        .position = { 0.5f, -0.5f, 0.0f, 1.0f },
        .color    = { 0.0f, 1.0f, 0.0f, 1.0f },
        .texCoord = { 0.0f, 0.0f },
    },
    (Vertex) {
        .position = { 0.5f, 0.5f, 0.0f, 1.0f },
        .color    = { 0.0f, 0.0f, 1.0f, 1.0f },
        .texCoord = { 0.0f, 1.0f },
    },
    (Vertex) {
        .position = { -0.5f, 0.5f, 0.0f, 1.0f },
        .color    = { 1.0f, 1.0f, 1.0f, 1.0f },
        .texCoord = { 1.0f, 1.0f },
    },

    (Vertex) {
        .position = { -0.5f, -0.5f, 0.5f, 1.0f },
        .color    = { 1.0f, 0.0f, 0.0f, 1.0f },
        .texCoord = { 1.0f, 0.0f },
    },
    (Vertex) {
        .position = { 0.5f, -0.5f, 0.5f, 1.0f },
        .color    = { 0.0f, 1.0f, 0.0f, 1.0f },
        .texCoord = { 0.0f, 0.0f },
    },
    (Vertex) {
        .position = { 0.5f, 0.5f, 0.5f, 1.0f },
        .color    = { 0.0f, 0.0f, 1.0f, 1.0f },
        .texCoord = { 0.0f, 1.0f },
    },
    (Vertex) {
        .position = { -0.5f, 0.5f, 0.5f, 1.0f },
        .color    = { 1.0f, 1.0f, 1.0f, 1.0f },
        .texCoord = { 1.0f, 1.0f },
    },
};

static uint32_t indices[] = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4,
};

typedef struct {
    Mat4 view;
    Mat4 proj;

    Mat4 invView;
    Mat4 invProj;
} UniformBufferObject;

static bool VSync = true;

IFDEBUG(static VkDebugUtilsMessengerEXT debugMessenger);

static VkInstance instance;
static VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

static VkSurfaceKHR surface;
static VkSwapchainKHR swapChain;
static VkFormat swapChainImageFormat;
static VkExtent2D swapChainExtent;
static Images swapChainImages = {0};

static Images viewportImages = {0};

static VkRenderPass renderPass;

static VkDescriptorSetLayout descriptorSetLayout;
static VkPipelineLayout pipelineLayout;
static VkPipeline graphicsPipeline;

static VkDescriptorSetLayout globalUBODescriptorSetLayout;

static Framebuffers swapChainFramebuffers = {0};

static VkCommandPool commandPool;

static Image color;
static Image depth;

static Texture texture;

static BufferData vertexBuffer;
static BufferData indexBuffer;

static MappedBufferDatas uniformBuffers = {0};

static VkDescriptorPool descriptorPool;

static DescriptorSets globalUBODescriptorSets = {0};
static DescriptorSets descriptorSets = {0};

static CommandBuffers commandBuffers = {0};

static Semaphores imageAvailableSemaphores = {0};
static Semaphores renderFinishedSemaphores = {0};
static Fences inFlightFences = {0};

static uint32_t currentFrame = 0;

static bool framebufferResized = false;

static BufferData aabbBuffer;

static VkVertexInputBindingDescription vulkan_get_vertex_binding_description()
{
    return (VkVertexInputBindingDescription) {
        .binding   = 0,
        .stride    = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
}

static void vulkan_get_attribute_descriptions(VertexInputAttributeDescriptions* vertexInputAttributeDescriptions)
{
    VkVertexInputAttributeDescription attributeDescriptor = {
        .binding  = 0,
        .location = 0,
        .format   = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset   = offsetof(Vertex, position),
    };
    list_append(*vertexInputAttributeDescriptions, attributeDescriptor);

    attributeDescriptor = (VkVertexInputAttributeDescription) {
        .binding  = 0,
        .location = 1,
        .format   = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset   = offsetof(Vertex, color),
    };
    list_append(*vertexInputAttributeDescriptions, attributeDescriptor);

    attributeDescriptor = (VkVertexInputAttributeDescription) {
        .binding  = 0,
        .location = 2,
        .format   = VK_FORMAT_R32G32_SFLOAT,
        .offset   = offsetof(Vertex, texCoord),
    };
    list_append(*vertexInputAttributeDescriptions, attributeDescriptor);
}

#if defined(VKDEBUG)

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    UNUSED(messageSeverity);
    UNUSED(messageType);
    UNUSED(pUserData);

    if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        log_error("Vulkan validation layer: %s", pCallbackData->pMessage);
    else
        log_debug("Vulkan validation layer: %s", pCallbackData->pMessage);
    
    return VK_FALSE;
}

static void vulkan_populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT* createInfo)
{
    *createInfo = (VkDebugUtilsMessengerCreateInfoEXT) {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = vulkan_debug_callback,
    };
}

VkResult vulkan_create_debug_utils_messenger_EXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func == NULL)
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

static bool vulkan_check_validation_layer_support()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    typedef struct {
        VkLayerProperties* items;
        size_t             count;
        size_t             capacity;
    } LayerProperties;

    LayerProperties availableLayers = {0};
    list_alloc(availableLayers, layerCount);

    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.items);
    availableLayers.count = layerCount;
    
    bool result = true;
    for (size_t i = 0; i < ARRAYLEN(validationLayers); ++i)
    {
        const char* layerName = validationLayers[i];
        bool layerFound = false;

        for (size_t j = 0; j < availableLayers.count; ++j)
        {
            const char* availableLayerName = availableLayers.items[j].layerName;
            if (strcmp(layerName, availableLayerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
            finalize(false);
    }
    
finalize:
    list_destroy(availableLayers);
    return result;
}

#endif

void vulkan_get_required_extensions(Extensions* extensions)
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    for(size_t i = 0; i < glfwExtensionCount; ++i)
        list_append(*extensions, glfwExtensions[i]);
    
    // Enable Validation Layers
    IFDEBUG({
        const char* ext = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        list_append(*extensions, ext);
    });
}

#if defined(VKDEBUG)

static void vulkan_destroy_debug_utils_messenger_EXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func =
        (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func == NULL)
        return;
    
    func(instance, debugMessenger, pAllocator);
}

#endif

static bool vulkan_create_instance(const char* title)
{
    // Enable Validation Layers
    IFDEBUG({
        if(!vulkan_check_validation_layer_support())
        {
            log_error("Vulkan validation layers requested, but not available!");
            return false;
        }
    });

    VkApplicationInfo appInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = title,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "Vulkan Engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_1,
    };
    
    VkInstanceCreateInfo createInfo = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = NULL,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = 0,
        .enabledExtensionCount   = 0,
    };

    Extensions extensions = {0};
    vulkan_get_required_extensions(&extensions);
    createInfo.enabledExtensionCount   = extensions.count;
    createInfo.ppEnabledExtensionNames = extensions.items;

#if defined(VKDEBUG)
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};

    createInfo.enabledLayerCount = ARRAYLEN(validationLayers);
    createInfo.ppEnabledLayerNames = validationLayers;

    vulkan_populate_debug_messenger_create_info(&debugCreateInfo);

    VkValidationFeatureEnableEXT validationFeatureEnables[] = {
        VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
    };
    VkValidationFeaturesEXT validationFeatures = {
        .sType                         = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .pNext                         = &debugCreateInfo,
        .enabledValidationFeatureCount = 1,
        .pEnabledValidationFeatures    = validationFeatureEnables,
    };

    createInfo.pNext = &validationFeatures;
#endif

    VKCHECK(vkCreateInstance(&createInfo, NULL, &instance));
    return true;
}

#if defined(VKDEBUG)

static bool vulkan_setup_debug_messenger()
{
    IFRELEASE({
        return true;
    });

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
    vulkan_populate_debug_messenger_create_info(&createInfo);

    VKCHECK(vulkan_create_debug_utils_messenger_EXT(instance, &createInfo, NULL, &debugMessenger));
    return true;
}

#endif

static bool vulkan_create_surface()
{
    VKCHECK(glfwCreateWindowSurface(instance, window_get_handle(), NULL, &surface));
    return true;
}

static bool vulkan_find_queue_families(VkPhysicalDevice device, QueueFamilyIndices* queueFamilyIndices)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    typedef struct {
        VkQueueFamilyProperties* items;
        size_t                   count;
        size_t                   capacity;
    } QueueFamilyProperties;
    QueueFamilyProperties queueFamilyProperties = {0};
    list_alloc(queueFamilyProperties, queueFamilyCount);
    
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties.items);
    queueFamilyProperties.count = queueFamilyCount;

    bool hasGraphicsFamily = false, hasPresentFamily = false;

    for(size_t i = 0; i < queueFamilyProperties.count; ++i)
    {
        VkQueueFamilyProperties* queueFamily = &queueFamilyProperties.items[i];
        if (queueFamily->queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queueFamilyIndices->graphicsFamily = i;
            hasGraphicsFamily = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if(presentSupport)
        {
            queueFamilyIndices->presentFamily = i;
            hasPresentFamily = true;
        }

        if(hasGraphicsFamily && hasPresentFamily)
            return true;
    }
    
    return false;
}

static bool vulkan_check_device_extension_support(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

    typedef struct {
        VkExtensionProperties* items;
        size_t                 count;
        size_t                 capacity;
    } ExtensionProperties;
    ExtensionProperties extensionProperties = {0};
    list_alloc(extensionProperties, extensionCount);
    
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, extensionProperties.items);
    extensionProperties.count = extensionCount;

    typedef struct {
        const char** items;
        size_t       count;
        size_t       capacity;
    } ExtensionPropertiesSet;
    ExtensionPropertiesSet extensionPropertiesSet = {0};

    // Unique append required extensions
    for(size_t i = 0; i < ARRAYLEN(deviceExensions); ++i)
    {
        const char* item = deviceExensions[i];

        bool founded = false;
        for(size_t j = 0; j < extensionPropertiesSet.count; ++j)
        {
            const char* ptr = extensionPropertiesSet.items[j] + j * sizeof(*extensionPropertiesSet.items);
            if(strcmp(ptr, item) == 0)
            {
                founded = true;
                break;
            }
        }

        if(!founded)
            list_append(extensionPropertiesSet, item);
    }

    // Check if all required extensions are available
    for (size_t i = 0; i < extensionPropertiesSet.count; ++i)
    {
        const char* item = extensionPropertiesSet.items[i];

        bool founded = false;
        for(size_t j = 0; j < extensionProperties.count; ++j)
        {
            if(strcmp(extensionProperties.items[j].extensionName, item) == 0)
            {
                founded = true;
                break;
            }
        }

        CHECK(founded);
    }

    return true;
}

static void vulkan_query_swap_chain_support(VkPhysicalDevice device, SwapChainSupportDetails* swapChainSupportDetails)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainSupportDetails->capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);

    if (formatCount != 0)
    {
        list_alloc(swapChainSupportDetails->formats, formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainSupportDetails->formats.items);
        swapChainSupportDetails->formats.count = formatCount;
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);

    if (presentModeCount != 0)
    {
        list_alloc(swapChainSupportDetails->presentModes, presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, swapChainSupportDetails->presentModes.items);
        swapChainSupportDetails->presentModes.count = presentModeCount;
    }
}

static bool vulkan_is_device_suitable(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    bool extensionsSupported = vulkan_check_device_extension_support(device);
    
    QueueFamilyIndices queueFanilyIndices;

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupportDetails = {0};
        vulkan_query_swap_chain_support(device, &swapChainSupportDetails);
        swapChainAdequate = swapChainSupportDetails.formats.count != 0 && swapChainSupportDetails.presentModes.count != 0;
    }
    
    return vulkan_find_queue_families(device, &queueFanilyIndices) && extensionsSupported &&
        swapChainAdequate  && deviceFeatures.geometryShader && deviceFeatures.samplerAnisotropy &&
        deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

static VkSampleCountFlagBits vulkan_get_max_usable_sample_count()
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts =
        physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

static bool vulkan_pick_physical_device()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if (deviceCount == 0)
    {
        log_error("Vulkan failed to find GPUs with Vulkan support!");
        return false;
    }

    typedef struct {
        VkPhysicalDevice* items;
        size_t            count;
        size_t            capacity;
    } PhysicalDevices;
    PhysicalDevices physicalDevices = {0};
    list_alloc(physicalDevices, deviceCount);

    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.items);
    physicalDevices.count = deviceCount;
    
    for (size_t i = 0; i < physicalDevices.count; ++i)
    {
        VkPhysicalDevice device = physicalDevices.items[i];
        if (vulkan_is_device_suitable(device))
        {
            physicalDevice = device;
            msaaSamples = vulkan_get_max_usable_sample_count();
            return true;
        }
    }
    
    log_error("Vulkan failed to find a suitable GPU!");
    return false;
}

static bool vulkan_create_logical_device()
{
    QueueFamilyIndices queueFanilyIndices;
    CHECK(vulkan_find_queue_families(physicalDevice, &queueFanilyIndices));
    
    typedef struct {
        VkDeviceQueueCreateInfo* items;
        size_t                   count;
        size_t                   capacity;
    } QueueCreateInfos;
    QueueCreateInfos queueCreateInfos = {0};
    
    typedef struct {
        uint32_t* items;
        size_t    count;
        size_t    capacity;
    } QueueFamiliesSet;
    QueueFamiliesSet queueFamiliesSet = {0};

    list_append_unique(queueFamiliesSet, queueFanilyIndices.graphicsFamily);
    list_append_unique(queueFamiliesSet, queueFanilyIndices.presentFamily);

    const float queuePriority = 1.0f;
    for(size_t i = 0; i < queueFamiliesSet.count; ++i)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamiliesSet.items[i],
            .queueCount       = 1,
            .pQueuePriorities = &queuePriority,
        };
        list_append(queueCreateInfos, queueCreateInfo);
    }
    
    VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR positionFetchFeatures = {
        .sType                   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR,
        .rayTracingPositionFetch = VK_TRUE,
    };

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {
        .sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .pNext              = &positionFetchFeatures,
        .rayTracingPipeline = VK_TRUE,
    };

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelStructFeatures = {
        .sType                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .pNext                 = &rayTracingPipelineFeatures,
        .accelerationStructure = VK_TRUE,
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures deviceAddressFeatures = {
        .sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .pNext               = &accelStructFeatures,
        .bufferDeviceAddress = VK_TRUE,
    };

    VkPhysicalDeviceDescriptorIndexingFeatures deviceDescriptorIndexingFeatures = {
        .sType                  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        .pNext                  = &deviceAddressFeatures,
        .runtimeDescriptorArray = VK_TRUE,
    };
    
    VkPhysicalDeviceFeatures deviceFeatures = {
        .samplerAnisotropy = VK_TRUE,
        .sampleRateShading = VK_TRUE,
        .shaderInt64       = VK_TRUE,
    };
    
    VkPhysicalDeviceFeatures2 deviceFeatures2 = {
        .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext    = &deviceDescriptorIndexingFeatures,
        .features = deviceFeatures,
    };

    VkDeviceCreateInfo createInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &deviceFeatures2,
        .queueCreateInfoCount    = queueCreateInfos.count,
        .pQueueCreateInfos       = queueCreateInfos.items,
        .enabledLayerCount       = 0,
        .enabledExtensionCount   = ARRAYLEN(deviceExensions),
        .ppEnabledExtensionNames = deviceExensions,
    };

    IFDEBUG({
        createInfo.enabledLayerCount   = ARRAYLEN(validationLayers);
        createInfo.ppEnabledLayerNames = validationLayers;
    });

    VKCHECK(vkCreateDevice(physicalDevice, &createInfo, NULL, &device));

    vkGetDeviceQueue(device, queueFanilyIndices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueFanilyIndices.presentFamily, 0, &presentQueue);
    return true;
}

static VkSurfaceFormatKHR vulkan_choose_swap_surface_format(const SurfaceFormatKHRs* availableFormats)
{
    VkFormat requestSurfaceImageFormat[] = {
        VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM
    };

    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

    for(size_t i = 0; i < ARRAYLEN(requestSurfaceImageFormat); ++i)
        for (size_t j = 0; j < availableFormats->count; ++j)
        {
            VkSurfaceFormatKHR surfaceFormat = availableFormats->items[j];
            if (surfaceFormat.format == requestSurfaceImageFormat[i] &&
                surfaceFormat.colorSpace == requestSurfaceColorSpace)
                return surfaceFormat;
        }
    
    return availableFormats->items[0];
}

static VkPresentModeKHR vulkan_choose_swap_present_mode(const PresentModeKHRs* availablePresentModes)
{
    if(VSync)
        return VK_PRESENT_MODE_FIFO_KHR;
    
    VkPresentModeKHR presentModes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };

    for (size_t i = 0; i < ARRAYLEN(presentModes); ++i)
        for (size_t j = 0; j < availablePresentModes->count; ++j)
        {
            VkPresentModeKHR presentMode = availablePresentModes->items[j];
            if (presentMode == presentModes[i])
                return presentMode;
        }
    
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D vulkan_choose_swap_extent(const VkSurfaceCapabilitiesKHR* capabilities)
{
    if (capabilities->currentExtent.width != UINT32_MAX)
        return capabilities->currentExtent;
    
    int width, height;
    glfwGetFramebufferSize(window_get_handle(), &width, &height);

    VkExtent2D actualExtent = {
        (uint32_t)width,
        (uint32_t)height,
    };

    actualExtent.width  = clamp(actualExtent.width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
    actualExtent.height = clamp(actualExtent.height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);
    return actualExtent;
}

static bool vulkan_create_swap_chain()
{
    SwapChainSupportDetails swapChainSupportDetails = {0};
    vulkan_query_swap_chain_support(physicalDevice, &swapChainSupportDetails);

    VkSurfaceFormatKHR surfaceFormat = vulkan_choose_swap_surface_format(&swapChainSupportDetails.formats);
    VkPresentModeKHR presentMode = vulkan_choose_swap_present_mode(&swapChainSupportDetails.presentModes);
    VkExtent2D extent = vulkan_choose_swap_extent(&swapChainSupportDetails.capabilities);

    uint32_t imageCount = swapChainSupportDetails.capabilities.minImageCount + 1;
    if (swapChainSupportDetails.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupportDetails.capabilities.maxImageCount)
        imageCount = swapChainSupportDetails.capabilities.maxImageCount;
    
    VkSwapchainCreateInfoKHR createInfo = {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = surface,
        .minImageCount    = imageCount,
        .imageFormat      = surfaceFormat.format,
        .imageColorSpace  = surfaceFormat.colorSpace,
        .imageExtent      = extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform     = swapChainSupportDetails.capabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = presentMode,
        .clipped          = VK_TRUE,
        .oldSwapchain     = VK_NULL_HANDLE,
    };

    if (swapChainSupportDetails.capabilities.supportedTransforms & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    if (swapChainSupportDetails.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    QueueFamilyIndices queueFamilyIndices;
    CHECK(vulkan_find_queue_families(physicalDevice, &queueFamilyIndices));
    
    uint32_t indices[] = {
        queueFamilyIndices.graphicsFamily,
        queueFamilyIndices.presentFamily,
    };

    if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily)
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = indices;
    }

    VKCHECK(vkCreateSwapchainKHR(device, &createInfo, NULL, &swapChain));
    
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, NULL);
    VkImage* images = (VkImage*) malloc(imageCount * sizeof(VkImage));
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, images);

    list_alloc(swapChainImages, imageCount);
    for(size_t i = 0; i < imageCount; ++i)
    {
        Image img = {
            .image = images[i],
        };
        list_append(swapChainImages, img);
    }

    free(images);

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
    return true;
}

static bool vulkan_create_image_views()
{
    for (size_t i = 0; i < swapChainImages.count; i++)
        vulkan_create_image_view(swapChainImages.items[i].image, swapChainImageFormat,
            VK_IMAGE_ASPECT_COLOR_BIT, 1, &swapChainImages.items[i].view);

    return true;
}

static bool vulkan_create_viewport_image(VkCommandPool commandPool)
{
    list_alloc(viewportImages, swapChainImages.count);
    viewportImages.count = swapChainImages.count;

    for (uint32_t i = 0; i < swapChainImages.count; i++)
    {
        CHECK(vulkan_create_image(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT,
            VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &viewportImages.items[i]));

        VkCommandBuffer commandBuffer;
        CHECK(vulkan_begin_single_time_commands(commandPool, &commandBuffer));

        VkImageMemoryBarrier barrier = {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask       = VK_ACCESS_TRANSFER_READ_BIT,
            .dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = viewportImages.items[i].image,
            .subresourceRange    = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1
            }
        };

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, NULL,
            0, NULL,
            1, &barrier);
        
        CHECK(vulkan_end_single_time_commands(commandPool, commandBuffer));
    }
    return true;
}

static bool vulkan_create_viewport_image_views()
{
    for (size_t i = 0; i < viewportImages.count; i++)
        vulkan_create_image_view(viewportImages.items[i].image, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1,
            &viewportImages.items[i].view);
    
    return true;
}

static bool vulkan_find_supported_format(const Formats* candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkFormat* format)
{
    for (size_t i = 0; i < candidates->count; ++i)
    {
        *format = candidates->items[i];

        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, *format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return true;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return true;
    }

    ASSERT_MSG(false, "Vulkan failed to find supported format!");
    return false;
}

static bool vulkan_find_depth_format(VkFormat* format)
{
    VkFormat fms[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
    Formats formats = {
        .items = fms,
        .count = ARRAYLEN(fms),
    };

    return vulkan_find_supported_format(&formats, VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, format
    );
}

// static bool vulkan_has_stencil_component(VkFormat format)
// {
//     return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
// }

static bool vulkan_create_color_resources()
{
    VkFormat colorFormat = swapChainImageFormat;

    CHECK(vulkan_create_image(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &color));
    
    CHECK(vulkan_create_image_view(color.image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, &color.view));
    return true;
}

static bool vulkan_create_depth_resources()
{
    VkFormat depthFormat;
    vulkan_find_depth_format(&depthFormat);

    CHECK(vulkan_create_image(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depth));
    
    CHECK(vulkan_create_image_view(depth.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, &depth.view));
    return true;
}

static bool vulkan_create_render_pass()
{
    VkAttachmentDescription colorAttachment = {
        .format         = swapChainImageFormat,
        .samples        = msaaSamples,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription depthAttachment = {
        .samples        = msaaSamples,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    CHECK(vulkan_find_depth_format(&depthAttachment.format));

    VkAttachmentDescription colorAttachmentResolve = {
        .format         = swapChainImageFormat,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depthAttachmentRef = {
        .attachment = 1,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference colorAttachmentResolveRef = {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = 1,
        .pColorAttachments       = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
        .pResolveAttachments     = &colorAttachmentResolveRef,
    };

    VkSubpassDependency dependency = {
        .srcSubpass    = VK_SUBPASS_EXTERNAL,
        .dstSubpass    = 0,
        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment, colorAttachmentResolve };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = ARRAYLEN(attachments),
        .pAttachments    = attachments,
        .subpassCount    = 1,
        .pSubpasses      = &subpass,
        .dependencyCount = 1,
        .pDependencies   = &dependency,
    };

    VKCHECK(vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass));
    return true;
}

static bool vulkan_create_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {
        .binding         = 0,
        .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    VkDescriptorSetLayoutBinding bindinds[] = { samplerLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = ARRAYLEN(bindinds),
        .pBindings    = bindinds,
    };

    VKCHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &descriptorSetLayout));
    return true;
}

static bool vulkan_create_global_ubo_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding = {
        .binding         = 0,
        .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings    = &uboLayoutBinding
    };

    VKCHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &globalUBODescriptorSetLayout));
    return true;
}

static bool vulkan_create_framebuffers()
{
    list_alloc(swapChainFramebuffers, swapChainImages.count);
    swapChainFramebuffers.count = swapChainImages.count;
    
    for (size_t i = 0; i < swapChainImages.count; ++i)
    {
        VkImageView attachments[] = {
            color.view,
            depth.view,
            swapChainImages.items[i].view,
        };

        VkFramebufferCreateInfo framebufferInfo = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = renderPass,
            .attachmentCount = ARRAYLEN(attachments),
            .pAttachments    = attachments,
            .width           = swapChainExtent.width,
            .height          = swapChainExtent.height,
            .layers          = 1,
        };
    
        VKCHECK(vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapChainFramebuffers.items[i]));
    }

    return true;
}

static bool vulkan_create_command_pool()
{
    QueueFamilyIndices queueFamilyIndices;
    CHECK(vulkan_find_queue_families(physicalDevice, &queueFamilyIndices));

    VkCommandPoolCreateInfo poolInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndices.graphicsFamily,
    };

    VKCHECK(vkCreateCommandPool(device, &poolInfo, NULL, &commandPool));
    return true;
}

static bool vulkan_create_uniform_buffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    list_alloc(uniformBuffers, MAX_FRAMES_IN_FLIGHT);
    uniformBuffers.count = MAX_FRAMES_IN_FLIGHT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vulkan_create_buffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0,
            &uniformBuffers.items[i].buffer, &uniformBuffers.items[i].memory);
        
        VKCHECK(vkMapMemory(device, uniformBuffers.items[i].memory, 0, bufferSize, 0, (void**)&uniformBuffers.items[i].map));
    }
    return true;
}

static bool vulkan_create_descriptor_pool()
{
    VkDescriptorPoolSize poolSizes[] = {
        [0] = (VkDescriptorPoolSize) {
            .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = (uint32_t) MAX_FRAMES_IN_FLIGHT,
        },
        [1] = (VkDescriptorPoolSize) {
            .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = (uint32_t) MAX_FRAMES_IN_FLIGHT,
        },
    };
    
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = ARRAYLEN(poolSizes),
        .pPoolSizes    = poolSizes,
        .maxSets       = (uint32_t) MAX_FRAMES_IN_FLIGHT * 2,
    };
    VKCHECK(vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptorPool));
    return true;
}

static bool vulkan_create_global_ubo_descriptor_sets()
{
    DescriptorSetLayouts layouts = {0};
    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        list_append(layouts, globalUBODescriptorSetLayout);
    
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = descriptorPool,
        .descriptorSetCount = (uint32_t) MAX_FRAMES_IN_FLIGHT,
        .pSetLayouts        = layouts.items,
    };

    list_alloc(globalUBODescriptorSets, MAX_FRAMES_IN_FLIGHT);
    globalUBODescriptorSets.count = MAX_FRAMES_IN_FLIGHT;
    
    VKCHECK(vkAllocateDescriptorSets(device, &allocInfo, globalUBODescriptorSets.items));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo = {
            .buffer = uniformBuffers.items[i].buffer,
            .offset = 0,
            .range  = sizeof(UniformBufferObject),
        };
        
        VkWriteDescriptorSet descriptorWrites[] = {
            [0] = (VkWriteDescriptorSet) {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = globalUBODescriptorSets.items[i],
                .dstBinding      = 0,
                .dstArrayElement = 0,
                .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo     = &bufferInfo,
            },
        };

        vkUpdateDescriptorSets(device, ARRAYLEN(descriptorWrites), descriptorWrites, 0, NULL);
    }
    return true;
}

static bool vulkan_create_descriptor_sets()
{
    DescriptorSetLayouts layouts = {0};
    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        list_append(layouts, descriptorSetLayout);
    
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = descriptorPool,
        .descriptorSetCount = (uint32_t) MAX_FRAMES_IN_FLIGHT,
        .pSetLayouts        = layouts.items,
    };

    list_alloc(descriptorSets, MAX_FRAMES_IN_FLIGHT);
    descriptorSets.count = MAX_FRAMES_IN_FLIGHT;
    
    VKCHECK(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.items));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorImageInfo imageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView   = texture.image.view,
            .sampler     = texture.sampler,
        };

        VkWriteDescriptorSet descriptorWrites[] = {
            [0] = (VkWriteDescriptorSet) {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = descriptorSets.items[i],
                .dstBinding      = 0,
                .dstArrayElement = 0,
                .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .pImageInfo      = &imageInfo,
            },
        };

        vkUpdateDescriptorSets(device, ARRAYLEN(descriptorWrites), descriptorWrites, 0, NULL);
    }
    return true;
}

static bool vulkan_create_command_buffers()
{
    list_alloc(commandBuffers, MAX_FRAMES_IN_FLIGHT);
    commandBuffers.count = MAX_FRAMES_IN_FLIGHT;

    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = commandBuffers.count,
    };
    
    VKCHECK(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.items));
    return true;
}

static bool vulkan_create_sync_objects()
{
    list_alloc(imageAvailableSemaphores, MAX_FRAMES_IN_FLIGHT);
    imageAvailableSemaphores.count = MAX_FRAMES_IN_FLIGHT;
    list_alloc(renderFinishedSemaphores, MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.count = MAX_FRAMES_IN_FLIGHT;
    list_alloc(inFlightFences, MAX_FRAMES_IN_FLIGHT);
    inFlightFences.count = MAX_FRAMES_IN_FLIGHT;

    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphores.items[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, NULL, &renderFinishedSemaphores.items[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, NULL, &inFlightFences.items[i]) != VK_SUCCESS)
        {
            log_error("Vulkan vkcheck error");
            return false;
        }
    }
    return true;
}

static bool vulkan_create_raytracing()
{
    CHECK(raytracing_init());
    
    char tempStr[1024];
    Timer t;
    
    {
        timer_start(&t);
        
        {
            uint32_t width = 1, height = 32, depth = 32;

            uint8_t volumeData[width * height * depth];
            for(uint32_t z = 0; z < depth; ++z)
                for(uint32_t y = 0; y < height; ++y)
                    for(uint32_t x = 0; x < width; ++x)
                        volumeData[x + y * width + z * (width * height)] = 5;

            CHECK(raytracing_add_volume_geometry(commandPool, width, height, depth, volumeData));
        }
        {
            uint32_t width = 1, height = 32, depth = 32;

            uint8_t volumeData[width * height * depth];
            for(uint32_t z = 0; z < depth; ++z)
                for(uint32_t y = 0; y < height; ++y)
                    for(uint32_t x = 0; x < width; ++x)
                        volumeData[x + y * width + z * (width * height)] = 6;

            CHECK(raytracing_add_volume_geometry(commandPool, width, height, depth, volumeData));
        }
        {
            uint32_t width = 32, height = 1, depth = 32;

            uint8_t volumeData[width * height * depth];
            for(uint32_t z = 0; z < depth; ++z)
                for(uint32_t y = 0; y < height; ++y)
                    for(uint32_t x = 0; x < width; ++x)
                        volumeData[x + y * width + z * (width * height)] = 7;

            CHECK(raytracing_add_volume_geometry(commandPool, width, height, depth, volumeData));
        }
        {
            uint32_t width = 32, height = 32, depth = 1;

            uint8_t volumeData[width * height * depth];
            for(uint32_t z = 0; z < depth; ++z)
                for(uint32_t y = 0; y < height; ++y)
                    for(uint32_t x = 0; x < width; ++x)
                        volumeData[x + y * width + z * (width * height)] = 1;

            CHECK(raytracing_add_volume_geometry(commandPool, width, height, depth, volumeData));
        }
        {
            uint32_t width = 8, height = 8, depth = 8;

            uint8_t volumeData[width * height * depth];
            for(uint32_t z = 0; z < depth; ++z)
                for(uint32_t y = 0; y < height; ++y)
                    for(uint32_t x = 0; x < width; ++x)
                    {
                        uint32_t i = x + y * width + z * (width * height);
                        volumeData[i] = (i % 3) + 2;
                    }

            CHECK(raytracing_add_volume_geometry(commandPool, width, height, depth, volumeData));
        }

        raytracing_add_triangle_geometry(vertexBuffer.buffer, indexBuffer.buffer,
            ARRAYLEN(vertices), sizeof(Vertex), ARRAYLEN(indices));

        timer_stop(&t);
        time_to_str(tempStr, timer_get_ns(&t));
        log_trace("Raytracing added geometries in %s", tempStr);
    }

    CHECK(raytracing_create_geometries_address_buffer(commandPool));

    {
        timer_start(&t);

        CHECK(raytracing_create_bottom_layer(commandPool));

        timer_stop(&t);
        time_to_str(tempStr, timer_get_ns(&t));
        log_trace("Raytracing building bottom layer in %s", tempStr);
    }

    {
        Mat4 transform;
        VkTransformMatrixKHR outTransform;

        size_t instances = 0;

        float scale = 0.2f;

        {
            Vec3 p = { 32.0f + 5.0f, 0.0f, 0.0f };

            mat4_identity(&transform);
            mat4_scale(&transform, &(Vec3){ scale, scale, scale });
            mat4_translate(&transform, &p, &transform);

            mat4_to_vk_transform(&transform, &outTransform);

            raytracing_add_volume_instance(0, &outTransform);
            ++instances;
        }
        {
            Vec3 p = { 5.0f, 0.0f, 0.0f };

            mat4_identity(&transform);
            mat4_scale(&transform, &(Vec3){ scale, scale, scale });
            mat4_translate(&transform, &p, &transform);

            mat4_to_vk_transform(&transform, &outTransform);

            raytracing_add_volume_instance(1, &outTransform);
            ++instances;
        }
        {
            // Vec3 p = { 5.0f, 0.0f, 0.0f };
            
            // mat4_identity(&transform);
            // mat4_scale(&transform, &(Vec3){ scale, scale, scale });
            // mat4_translate(&transform, &p, &transform);
            
            // mat4_to_vk_transform(&transform, &outTransform);
            
            // raytracing_add_volume_instance(2, &outTransform);
            // ++instances;
        }
        {
            // Vec3 p = { 5.0f, 32.0f, 0.0f };
            
            // mat4_identity(&transform);
            // mat4_scale(&transform, &(Vec3){ scale, scale, scale });
            // mat4_translate(&transform, &p, &transform);
            
            // mat4_to_vk_transform(&transform, &outTransform);
            
            // raytracing_add_volume_instance(2, &outTransform);
            // ++instances;
        }
        {
            Vec3 p = { 5.0f, 0.0f, 32.0f };

            mat4_identity(&transform);
            mat4_scale(&transform, &(Vec3){ scale, scale, scale });
            mat4_translate(&transform, &p, &transform);

            mat4_to_vk_transform(&transform, &outTransform);

            raytracing_add_volume_instance(3, &outTransform);
            ++instances;
        }
        {
            Vec3 p = { 5.0f, 0.0f, 0.0f };

            mat4_identity(&transform);
            mat4_scale(&transform, &(Vec3){ scale, scale, scale });
            mat4_translate(&transform, &p, &transform);

            mat4_to_vk_transform(&transform, &outTransform);

            raytracing_add_volume_instance(3, &outTransform);
            ++instances;
        }
        {
            Vec3 p = { 12.0f + 5.0f, 12.0f, 16.0f };

            mat4_identity(&transform);
            mat4_scale(&transform, &(Vec3){ scale, scale, scale });
            mat4_translate(&transform, &p, &transform);

            mat4_to_vk_transform(&transform, &outTransform);

            raytracing_add_volume_instance(4, &outTransform);
            ++instances;
        }

        {
            Vec3 pos = { 0.0f, 32.0f, 0.0f };

            float radius = 10;
            for(float z = -radius; z <= radius; ++z)
                for(float x = -radius; x <= radius; ++x)
                {
                    Vec3 p = { x * 32.0f, 0.0f, z * 32.0f };
                    vec3_add(&pos, &p, &p);

                    mat4_identity(&transform);
                    mat4_scale(&transform, &(Vec3){ scale, scale, scale });
                    mat4_translate(&transform, &p, &transform);

                    mat4_to_vk_transform(&transform, &outTransform);

                    raytracing_add_volume_instance(2, &outTransform);
                    ++instances;
                }

            pos = (Vec3) { 0.0f, 0.0f, 0.0f };
            
            for(float z = -radius; z <= radius; ++z)
                for(float x = -radius; x <= radius; ++x)
                {
                    Vec3 p = { x * 32.0f, 0.0f, z * 32.0f };
                    vec3_add(&pos, &p, &p);

                    mat4_identity(&transform);
                    mat4_scale(&transform, &(Vec3){ scale, scale, scale });
                    mat4_translate(&transform, &p, &transform);

                    mat4_to_vk_transform(&transform, &outTransform);

                    raytracing_add_volume_instance(1, &outTransform);
                    ++instances;
                }
        }

        Vec3 pos;

        float size = 1;
        for(pos.z = 0; pos.z < size; ++pos.z)
            for(pos.y = 0; pos.y > -size; --pos.y)
                for(pos.x = 0; pos.x < size; ++pos.x)
                {
                    mat4_identity(&transform);
                    mat4_translate(&transform, &pos, &transform);
                    
                    mat4_to_vk_transform(&transform, &outTransform);

                    raytracing_add_triangle_instance(5, &outTransform);
                    ++instances;
                }

        log_trace("Raytracing added instances: %zu", instances);
    }

    {
        timer_start(&t);

        CHECK(raytracing_create_top_layer(commandPool));

        timer_stop(&t);
        time_to_str(tempStr, timer_get_ns(&t));
        log_trace("Raytracing building top layer in %s", tempStr);
    }

    CHECK(raytracing_create_descriptors(viewportImages, &texture));
    CHECK(raytracing_create_pipeline(globalUBODescriptorSetLayout));
    CHECK(raytracing_create_shader_binding_table());
    return true;
}

bool vulkan_init(const char* title)
{
    CHECK(vulkan_create_instance(title));
    IFDEBUG(CHECK(vulkan_setup_debug_messenger()));
    CHECK(vulkan_create_surface());
    CHECK(vulkan_pick_physical_device());
    CHECK(vulkan_create_logical_device());
    CHECK(vulkan_create_swap_chain());
    CHECK(vulkan_create_image_views());

    CHECK(vulkan_create_render_pass());
    CHECK(vulkan_create_descriptor_set_layout());

    CHECK(vulkan_create_global_ubo_descriptor_set_layout());
    
    {
        VertexInputAttributeDescriptions vertexInputAttributeDescriptions = {0};
        vulkan_get_attribute_descriptions(&vertexInputAttributeDescriptions);

        VkDescriptorSetLayout descriptorSetLayouts[] = {
            globalUBODescriptorSetLayout,
            descriptorSetLayout,
        };

        CHECK(vulkan_shader_create_graphics_pipeline("res/shaders/base.vert.spv", "res/shaders/base.frag.spv",
            vulkan_get_vertex_binding_description(), vertexInputAttributeDescriptions, swapChainExtent, msaaSamples,
            descriptorSetLayouts, ARRAYLEN(descriptorSetLayouts), &pipelineLayout, renderPass, &graphicsPipeline));
    }

    CHECK(vulkan_create_command_pool());

    CHECK(vulkan_create_viewport_image(commandPool));
    CHECK(vulkan_create_viewport_image_views());

    CHECK(vulkan_create_color_resources());
    CHECK(vulkan_create_depth_resources());
    CHECK(vulkan_create_framebuffers());

    CHECK(vulkan_create_texture_image("res/textures/texture.jpg", commandPool, VK_IMAGE_USAGE_SAMPLED_BIT, true, &texture));
    CHECK(vulkan_create_texture_image_view(&texture));
    CHECK(vulkan_create_texture_sampler(&texture));

    CHECK(vulkan_create_data_buffer(commandPool, vertices, sizeof(vertices),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, &vertexBuffer.buffer, &vertexBuffer.memory));

    CHECK(vulkan_create_data_buffer(commandPool, indices, sizeof(indices),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, &indexBuffer.buffer, &indexBuffer.memory));

    CHECK(vulkan_create_uniform_buffers());

    CHECK(vulkan_create_descriptor_pool());

    CHECK(vulkan_create_global_ubo_descriptor_sets());
    CHECK(vulkan_create_descriptor_sets());

    CHECK(vulkan_create_command_buffers());

    CHECK(vulkan_create_sync_objects());

    CHECK(vulkan_create_raytracing());
    
    log_info("Vulkan infos:");
    
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    log_info("    Device: %s", properties.deviceName);
    log_info("    Api Version: %u.%u.%u",
        VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion));
    return true;
}

static void vulkan_cleanup_swap_chain()
{
    for(size_t i = 0; i < viewportImages.count; ++i)
        DeleteImage(viewportImages.items[i]);
    
    DeleteImage(color);
    DeleteImage(depth);

    for (size_t i = 0; i < swapChainFramebuffers.count; i++)
        vkDestroyFramebuffer(device, swapChainFramebuffers.items[i], NULL);

    for (size_t i = 0; i < swapChainImages.count; i++)
        vkDestroyImageView(device, swapChainImages.items[i].view, NULL);

    vkDestroySwapchainKHR(device, swapChain, NULL);
}

static bool vulkan_recreate_swap_chain()
{
    VKCHECK(vkDeviceWaitIdle(device));

    vulkan_cleanup_swap_chain();

    CHECK(vulkan_create_swap_chain());
    CHECK(vulkan_create_image_views());

    CHECK(vulkan_create_viewport_image(commandPool));
    CHECK(vulkan_create_viewport_image_views());

    CHECK(vulkan_create_color_resources());
    CHECK(vulkan_create_depth_resources());
    CHECK(vulkan_create_framebuffers());

    CHECK(raytracing_update_descriptor_sets(viewportImages, &texture));
    return true;
}

void vulkan_destroy()
{
    vkDeviceWaitIdle(device);

    raytracing_destroy();

    DeleteBuffer(aabbBuffer);

    vulkan_cleanup_swap_chain();

    vkDestroyPipeline(device, graphicsPipeline, NULL);
    vkDestroyPipelineLayout(device, pipelineLayout, NULL);
    vkDestroyRenderPass(device, renderPass, NULL);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        DeleteMappedBuffer(uniformBuffers.items[i]);

    vkDestroyDescriptorPool(device, descriptorPool, NULL);

    DeleteTexture(texture);

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);

    vkDestroyDescriptorSetLayout(device, globalUBODescriptorSetLayout, NULL);

    DeleteBuffer(indexBuffer);
    DeleteBuffer(vertexBuffer);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(device, renderFinishedSemaphores.items[i], NULL);
        vkDestroySemaphore(device, imageAvailableSemaphores.items[i], NULL);
        vkDestroyFence(device, inFlightFences.items[i], NULL);
    }
    
    vkDestroyCommandPool(device, commandPool, NULL);
    
    vkDestroyDevice(device, NULL);
    
    IFDEBUG(vulkan_destroy_debug_utils_messenger_EXT(instance, debugMessenger, NULL));

    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
}

void vulkan_resize()
{
    framebufferResized = true;
}

static bool vulkan_record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    VKCHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    /*
    VkClearValue clearColor[] = {
        [0] = (VkClearValue) {
            .color = {
                .float32 = { 0.1f, 0.1f, 0.1f, 1.0f }
            }
        },
        [1] = (VkClearValue) {
            .depthStencil = { 1.0f, 0 },
        },
    };

    VkRenderPassBeginInfo renderPassInfo = {
        .sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass        = renderPass,
        .framebuffer       = swapChainFramebuffers.items[imageIndex],
        .renderArea.offset = { 0, 0 },
        .renderArea.extent = swapChainExtent,
        .clearValueCount   = ARRAYLEN(clearColor),
        .pClearValues      = clearColor,
    };
    */

    // Render Quad
    /*
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkViewport viewport = {
            .x        = 0.0f,
            .y        = 0.0f,
            .width    = (float) swapChainExtent.width,
            .height   = (float) swapChainExtent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = {
            .offset = { 0, 0 },
            .extent = swapChainExtent,
        };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
            0, 1, &globalUBODescriptorSets.items[currentFrame], 0, NULL);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
            1, 1, &descriptorSets.items[currentFrame], 0, NULL);

        VkBuffer vertexBuffers[] = { vertexBuffer.buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UIN32);

        vkCmdDrawIndexed(commandBuffer, ARRAYLEN(indices), 1, 0, 0, 0);
    }
    vkCmdEndRenderPass(commandBuffer);
    */

    // Raytracing
    {
        VkImageSubresourceRange subresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        };

        // Viewport image to access Shader Write
        VkImageMemoryBarrier barrier = {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask       = VK_ACCESS_NONE,
            .dstAccessMask       = VK_ACCESS_SHADER_WRITE_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = viewportImages.items[currentFrame].image,
            .subresourceRange    = subresourceRange
        };

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, NULL,
            0, NULL,
            1, &barrier);

        raytracer_render(commandBuffer, currentFrame,
            swapChainExtent.width, swapChainExtent.height, globalUBODescriptorSets.items[currentFrame]);
        
        // Viewport image to Transfer Src
        barrier = (VkImageMemoryBarrier) {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask       = VK_ACCESS_NONE,
            .dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = viewportImages.items[currentFrame].image,
            .subresourceRange    = subresourceRange
        };

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, NULL,
            0, NULL,
            1, &barrier);

        // Swapchain to Transfer dst
        barrier = (VkImageMemoryBarrier) {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask       = VK_ACCESS_NONE,
            .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = swapChainImages.items[imageIndex].image,
            .subresourceRange    = subresourceRange,
        };

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, NULL,
            0, NULL,
            1, &barrier);
        
        VkImageCopy blit = {
            .srcSubresource = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel       = 0,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
            .srcOffset = { 0, 0, 0 },
            .dstSubresource = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel       = 0,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
            .dstOffset = { 0, 0, 0 },
            .extent = { swapChainExtent.width, swapChainExtent.height, 1.0f },
        };
        vkCmdCopyImage(commandBuffer, viewportImages.items[currentFrame].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            swapChainImages.items[imageIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit);

        // Swapchain to Shadered present
        barrier = (VkImageMemoryBarrier) {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask       = VK_ACCESS_NONE,
            .dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = swapChainImages.items[imageIndex].image,
            .subresourceRange    = subresourceRange,
        };

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, NULL,
            0, NULL,
            1, &barrier);
    }

    VKCHECK(vkEndCommandBuffer(commandBuffer));
    return true;
}

static void vulkan_update_uniform_buffer(uint32_t currentImage)
{
    memcpy(uniformBuffers.items[currentImage].map, camera_get_data(), sizeof(CameraData));
}

bool vulkan_draw_frame()
{
    VKCHECK(vkWaitForFences(device, 1, &inFlightFences.items[currentFrame], VK_TRUE, UINT64_MAX));

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores.items[currentFrame],
        VK_NULL_HANDLE, &imageIndex);
    if(result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        framebufferResized = false;
        vulkan_recreate_swap_chain();
        return true;
    }
    else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        log_error("Vulkan acquire next image");
        return false;
    }

    VKCHECK(vkResetFences(device, 1, &inFlightFences.items[currentFrame]));

    VKCHECK(vkResetCommandBuffer(commandBuffers.items[currentFrame], 0));
    CHECK(vulkan_record_command_buffer(commandBuffers.items[currentFrame], imageIndex));
    
    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores.items[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores.items[currentFrame] };

    vulkan_update_uniform_buffer(currentFrame);

    VkSubmitInfo submitInfo = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = waitSemaphores,
        .pWaitDstStageMask    = waitStages,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &commandBuffers.items[currentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = signalSemaphores,
    };
    
    VKCHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences.items[currentFrame]));

    VkSwapchainKHR swapChains[] = { swapChain };

    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = signalSemaphores,
        .swapchainCount     = 1,
        .pSwapchains        = swapChains,
        .pImageIndices      = &imageIndex,
    };

    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
    {
        framebufferResized = false;
        vulkan_recreate_swap_chain();
    }
    else if (result != VK_SUCCESS)
    {
        log_error("Vulkan queue present");
        return false;
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    return true;
}
