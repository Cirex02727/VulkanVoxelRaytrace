#ifndef VULKAN_GLOBALS_H_
#define VULKAN_GLOBALS_H_

#include "core/list_types.h"

#include <vulkan/vulkan.h>

VkPhysicalDevice physicalDevice;
VkDevice device;

VkQueue graphicsQueue, presentQueue;

uint32_t maxRayRecursionDepth = 1;

#endif // VULKAN_GLOBALS_H_
