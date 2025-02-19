#include "vulkan_base.h"

extern VkDevice device;
extern VkPhysicalDevice physicalDevice;

extern VkQueue graphicsQueue;

uint32_t vulkan_find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    
    ASSERT_MSG(false, "Vulkan failed to find suitable memory type!");
    return -1;
}

bool vulkan_begin_single_time_commands(VkCommandPool commandPool, VkCommandBuffer* commandBuffer)
{
    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool        = commandPool,
        .commandBufferCount = 1,
    };

    VKCHECK(vkAllocateCommandBuffers(device, &allocInfo, commandBuffer));

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    VKCHECK(vkBeginCommandBuffer(*commandBuffer, &beginInfo));
    return true;
}

bool vulkan_end_single_time_commands(VkCommandPool commandPool, VkCommandBuffer commandBuffer)
{
    VKCHECK(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &commandBuffer,
    };

    VKCHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
    VKCHECK(vkQueueWaitIdle(graphicsQueue));

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    return true;
}

void mat4_to_vk_transform(Mat4* m, VkTransformMatrixKHR* out)
{
    out->matrix[0][0] = m->r0.x;
    out->matrix[0][1] = m->r1.x;
    out->matrix[0][2] = m->r2.x;
    out->matrix[0][3] = m->r3.x;
    out->matrix[1][0] = m->r0.y;
    out->matrix[1][1] = m->r1.y;
    out->matrix[1][2] = m->r2.y;
    out->matrix[1][3] = m->r3.y;
    out->matrix[2][0] = m->r0.z;
    out->matrix[2][1] = m->r1.z;
    out->matrix[2][2] = m->r2.z;
    out->matrix[2][3] = m->r3.z;
}
