#include "image.h"

extern VkDevice device;
extern VkPhysicalDevice physicalDevice;

bool vulkan_create_image(uint32_t width, uint32_t height, uint32_t mipLevels,
    VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties, Image* image)
{
    VkImageCreateInfo imageInfo = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .extent.width  = width,
        .extent.height = height,
        .extent.depth  = 1,
        .mipLevels     = mipLevels,
        .arrayLayers   = 1,
        .format        = format,
        .tiling        = tiling,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage         = usage,
        .samples       = numSamples,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
    };

    const VkPhysicalDeviceImageFormatInfo2 physicalDeviceImageFormatInfo2 = {
        .sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
        .format = format,
        .type   = VK_IMAGE_TYPE_2D,
        .tiling = tiling,
        .usage  = usage,
    };
    VkImageFormatProperties2 imageFormatProperties2 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
    };
    if(vkGetPhysicalDeviceImageFormatProperties2(physicalDevice, &physicalDeviceImageFormatInfo2,
        &imageFormatProperties2) == VK_ERROR_FORMAT_NOT_SUPPORTED)
    {
        log_error("Vulkan image error format not supported");
        return false;
    }

    VKCHECK(vkCreateImage(device, &imageInfo, NULL, &image->image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image->image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = vulkan_find_memory_type(memRequirements.memoryTypeBits, properties),
    };

    VKCHECK(vkAllocateMemory(device, &allocInfo, NULL, &image->memory));

    VKCHECK(vkBindImageMemory(device, image->image, image->memory, 0));
    return true;
}

bool vulkan_create_image_3d(uint32_t width, uint32_t height, uint32_t depth, VkFormat format, VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties, Image* image)
{
    VkImageCreateInfo imageInfo = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_3D,
        .extent.width  = width,
        .extent.height = height,
        .extent.depth  = depth,
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .format        = format,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage         = usage,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
    };

    const VkPhysicalDeviceImageFormatInfo2 physicalDeviceImageFormatInfo2 = {
        .sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
        .format = format,
        .type   = VK_IMAGE_TYPE_2D,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage  = usage,
    };
    VkImageFormatProperties2 imageFormatProperties2 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
    };
    if(vkGetPhysicalDeviceImageFormatProperties2(physicalDevice, &physicalDeviceImageFormatInfo2,
        &imageFormatProperties2) == VK_ERROR_FORMAT_NOT_SUPPORTED)
    {
        log_error("Vulkan image error format not supported");
        return false;
    }

    VKCHECK(vkCreateImage(device, &imageInfo, NULL, &image->image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image->image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = vulkan_find_memory_type(memRequirements.memoryTypeBits, properties),
    };

    VKCHECK(vkAllocateMemory(device, &allocInfo, NULL, &image->memory));

    VKCHECK(vkBindImageMemory(device, image->image, image->memory, 0));
    return true;
}

bool vulkan_transition_image_layout(VkCommandPool commandPool, VkImage image,
    VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer;
    if(!vulkan_begin_single_time_commands(commandPool, &commandBuffer))
        return false;

    VkImageMemoryBarrier barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image,
        .subresourceRange    = (VkImageSubresourceRange) {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = mipLevels,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
    };

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        log_error("Vulkan image unsupported layout transition!");
        return false;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
    );

    if(!vulkan_end_single_time_commands(commandPool, commandBuffer))
        return false;
    
    return true;
}

bool vulkan_copy_buffer_to_image(VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer;
    if(!vulkan_begin_single_time_commands(commandPool, &commandBuffer))
        return false;

    VkBufferImageCopy region = {
        .bufferOffset      = 0,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource  = (VkImageSubresourceLayers) {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel       = 0,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = (VkExtent3D) {
            width,
            height,
            1,
        }
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    if(!vulkan_end_single_time_commands(commandPool, commandBuffer))
        return false;
    
    return true;
}

bool vulkan_copy_buffer_to_image_3d(VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth)
{
    VkCommandBuffer commandBuffer;
    if(!vulkan_begin_single_time_commands(commandPool, &commandBuffer))
        return false;

    VkBufferImageCopy region = {
        .bufferOffset      = 0,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource  = (VkImageSubresourceLayers) {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel       = 0,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = (VkExtent3D) {
            width,
            height,
            depth,
        }
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    if(!vulkan_end_single_time_commands(commandPool, commandBuffer))
        return false;
    
    return true;
}

bool vulkan_create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageView* imageView)
{
    VkImageViewCreateInfo viewInfo = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = image,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = format,
        .subresourceRange = (VkImageSubresourceRange) {
            .aspectMask     = aspectFlags,
            .baseMipLevel   = 0,
            .levelCount     = mipLevels,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
    };

    VKCHECK(vkCreateImageView(device, &viewInfo, NULL, imageView));
    return true;
}

bool vulkan_create_image_view_3d(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView)
{
    VkImageViewCreateInfo viewInfo = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = image,
        .viewType         = VK_IMAGE_VIEW_TYPE_3D,
        .format           = format,
        .subresourceRange = (VkImageSubresourceRange) {
            .aspectMask     = aspectFlags,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
    };

    VKCHECK(vkCreateImageView(device, &viewInfo, NULL, imageView));
    return true;
}

bool vulkan_generate_mipmaps(VkCommandPool commandPool, VkImage image, VkFormat imageFormat,
    int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

    ASSERT_MSG(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT,
        "Vulkan texture image format does not support linear blitting!");
    
    VkCommandBuffer commandBuffer;
    if(!vulkan_begin_single_time_commands(commandPool, &commandBuffer))
        return false;

    VkImageMemoryBarrier barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image               = image,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange    = (VkImageSubresourceRange) {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseArrayLayer = 0,
            .layerCount     = 1,
            .levelCount     = 1,
        },
    };

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, NULL,
            0, NULL,
            1, &barrier);

        VkImageBlit blit = {
            .srcOffsets = {
                [0] = { 0, 0, 0 },
                [1] = {mipWidth, mipHeight, 1},
            },
            .srcSubresource = (VkImageSubresourceLayers) {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .dstOffsets = {
                [0] = {0, 0, 0},
                [1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 },
            },
            .dstSubresource = (VkImageSubresourceLayers) {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, NULL,
            0, NULL,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, NULL,
        0, NULL,
        1, &barrier);

    if(!vulkan_end_single_time_commands(commandPool, commandBuffer))
        return false;
    
    return true;
}
