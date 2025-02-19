#ifndef BUFFER_H_
#define BUFFER_H_

#include "vulkan_base.h"

struct BufferData{
    VkBuffer       buffer;
    VkDeviceMemory memory;
};

struct MappedBufferData{
    VkBuffer       buffer;
    VkDeviceMemory memory;
    void*          map;
};

#define DeleteBuffer(buff) {                        \
        vkFreeMemory(device, buff.memory, NULL);    \
        vkDestroyBuffer(device, buff.buffer, NULL); \
    }

#define DeleteMappedBuffer(buff) {                  \
        vkUnmapMemory(device, buff.memory);         \
        vkFreeMemory(device, buff.memory, NULL);    \
        vkDestroyBuffer(device, buff.buffer, NULL); \
    }

bool vulkan_create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkMemoryAllocateFlags nextFlags, VkBuffer* buffer, VkDeviceMemory* memory);

bool vulkan_copy_buffer(VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

bool vulkan_create_data_buffer(VkCommandPool commandPool, const void* data, VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkMemoryAllocateFlags nextFlags, VkBuffer* buffer, VkDeviceMemory* memory);

bool vulkan_create_mapped_data_buffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkMemoryAllocateFlags nextFlags, VkBuffer* buffer, VkDeviceMemory* memory, void** map);

#endif // BUFFER_H_
