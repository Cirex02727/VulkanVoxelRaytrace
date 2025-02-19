#include "texture.h"

#include "stb_image.h"

#include <string.h>
#include <math.h>

extern VkDevice device;
extern VkPhysicalDevice physicalDevice;

bool vulkan_create_texture_image(const char* filepath, VkCommandPool commandPool,
    VkImageUsageFlags usage, bool mipmaps, Texture* texture)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filepath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
    {
        log_error("Vulkan failed to load texture image!");
        return false;
    }

    texture->mipLevels = mipmaps ? (uint32_t) floorf(log2f(fmaxf(texWidth, texHeight))) + 1 : 1;

    BufferData stagingBuffer;
    if(!vulkan_create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0, &stagingBuffer.buffer, &stagingBuffer.memory))
        return false;

    void* data;
    vkMapMemory(device, stagingBuffer.memory, 0, imageSize, 0, &data);
    memcpy(data, pixels, (size_t) imageSize);
    vkUnmapMemory(device, stagingBuffer.memory);

    stbi_image_free(pixels);

    if(!vulkan_create_image(texWidth, texHeight, texture->mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texture->image))
        return false;

    if(!vulkan_transition_image_layout(commandPool, texture->image.image,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture->mipLevels))
        return false;
    
    if(!vulkan_copy_buffer_to_image(commandPool, stagingBuffer.buffer, texture->image.image, (uint32_t) texWidth, (uint32_t) texHeight))
        return false;
    
    DeleteBuffer(stagingBuffer);

    if(!vulkan_generate_mipmaps(commandPool, texture->image.image, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, texture->mipLevels))
        return false;
    
    return true;
}

bool vulkan_upload_texture_buffer_3d(void* data, uint32_t width, uint32_t height, uint32_t depth,
    VkCommandPool commandPool, Texture* texture)
{
    size_t size = width * height * depth;

    BufferData stagingBuffer;
    if(!vulkan_create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0, &stagingBuffer.buffer, &stagingBuffer.memory))
        return false;
    
    void* stagingData;
    vkMapMemory(device, stagingBuffer.memory, 0, size, 0, &stagingData);
    memcpy(stagingData, data, size);
    vkUnmapMemory(device, stagingBuffer.memory);
    
    if(!vulkan_transition_image_layout(commandPool, texture->image.image,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture->mipLevels))
        return false;
    
    if(!vulkan_copy_buffer_to_image_3d(commandPool, stagingBuffer.buffer, texture->image.image, width, height, depth))
        return false;
    
    DeleteBuffer(stagingBuffer);
    return true;
}

bool vulkan_create_texture_image_view(Texture* texture)
{
    return vulkan_create_image_view(texture->image.image, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT, texture->mipLevels, &texture->image.view);
}

bool vulkan_create_texture_sampler(Texture* texture)
{
    VkPhysicalDeviceProperties properties = {0};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo = {
        .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter               = VK_FILTER_LINEAR,
        .minFilter               = VK_FILTER_LINEAR,
        .addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable        = VK_TRUE,
        .maxAnisotropy           = properties.limits.maxSamplerAnisotropy,
        .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable           = VK_FALSE,
        .compareOp               = VK_COMPARE_OP_ALWAYS,
        .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .minLod                  = 0.0f,
        .maxLod                  = (float) texture->mipLevels,
        .mipLodBias              = 0.0f,
    };
    
    VKCHECK(vkCreateSampler(device, &samplerInfo, NULL, &texture->sampler));
    return true;
}
