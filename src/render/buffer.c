#include "buffer.h"

#include "vulkan_base.h"

#include <string.h>

extern VkDevice device;

bool vulkan_create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkMemoryAllocateFlags nextFlags, VkBuffer* buffer, VkDeviceMemory* memory)
{
    VkBufferCreateInfo bufferInfo = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VKCHECK(vkCreateBuffer(device, &bufferInfo, NULL, buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

    VkMemoryAllocateFlagsInfoKHR flagsInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR,
        .flags = nextFlags,
    };

    VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = nextFlags ? &flagsInfo : 0,
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = vulkan_find_memory_type(memRequirements.memoryTypeBits, properties),
    };

    VKCHECK(vkAllocateMemory(device, &allocInfo, NULL, memory));
    VKCHECK(vkBindBufferMemory(device, *buffer, *memory, 0));
    return true;
}

bool vulkan_copy_buffer(VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer;
    if(!vulkan_begin_single_time_commands(commandPool, &commandBuffer))
        return false;
    
    VkBufferCopy copyRegion = {
        .size = size,
    };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    if(!vulkan_end_single_time_commands(commandPool, commandBuffer))
        return false;
    
    return true;
}

bool vulkan_create_data_buffer(VkCommandPool commandPool, const void* data, VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkMemoryAllocateFlags nextFlags, VkBuffer* buffer, VkDeviceMemory* memory)
{
    BufferData stagingBuffer;
    if(!vulkan_create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        0, &stagingBuffer.buffer, &stagingBuffer.memory))
        return false;
        
    void* mappedData;
    VKCHECK(vkMapMemory(device, stagingBuffer.memory, 0, size, 0, &mappedData));
    memcpy(mappedData, data, size);
    vkUnmapMemory(device, stagingBuffer.memory);
    
    if(!vulkan_create_buffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, properties, nextFlags, buffer, memory))
        return false;
    
    if(!vulkan_copy_buffer(commandPool, stagingBuffer.buffer, *buffer, size))
        return false;
    
    DeleteBuffer(stagingBuffer);
    return true;
}

bool vulkan_create_mapped_data_buffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkMemoryAllocateFlags nextFlags, VkBuffer* buffer, VkDeviceMemory* memory, void** map)
{
    if(!vulkan_create_buffer(size, usage, properties, nextFlags, buffer, memory))
        return false;
    
    VKCHECK(vkMapMemory(device, *memory, 0, size, 0, map));
    memcpy(*map, data, size);
    
    return true;
}
