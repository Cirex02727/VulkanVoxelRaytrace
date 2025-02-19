#ifndef VULKAN_BASE_H_
#define VULKAN_BASE_H_

#include "core/core.h"

#include <vulkan/vulkan.h>

#include <stdbool.h>

#if defined(VKDEBUG)

    #define VKCHECK(x) { VkResult res = (x); if (res != VK_SUCCESS) { log_error("Vulkan vkcheck error " #x ": %d", *(int*)&res); return false; } }
    #define CHECK(x) { if (!(x)) { log_error("Check error " #x); return false; } }

#else

    #define VKCHECK(x) { if ((x) != VK_SUCCESS) return false; }
    #define CHECK(x) { if (!(x)) return false; }

#endif

#define VK_DEVICE_PFN(device, funcName) {                                         \
        funcName = (PFN_vk##funcName)vkGetDeviceProcAddr(device, "vk" #funcName); \
        if (funcName == NULL) {                                                   \
            log_error("Vulkan failed to resolve function: " #funcName "\n");      \
            return false;                                                         \
        }                                                                         \
    }

uint32_t vulkan_find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties);

bool vulkan_begin_single_time_commands(VkCommandPool commandPool, VkCommandBuffer* commandBuffer);

bool vulkan_end_single_time_commands(VkCommandPool commandPool, VkCommandBuffer commandBuffer);

// ##### Utils #####

void mat4_to_vk_transform(Mat4* m, VkTransformMatrixKHR* out);

// ##### Utils #####

#endif // VULKAN_BASE_H_
