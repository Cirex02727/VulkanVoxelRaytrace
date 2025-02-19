#ifndef IMAGE_H_
#define IMAGE_H_

#include "vulkan_base.h"
#include "buffer.h"

struct Image {
    VkImage image;
    VkDeviceMemory memory;

    VkImageView view;
};

#define DeleteImage(img) {                                \
        if((img).view)                                    \
            vkDestroyImageView(device, (img).view, NULL); \
                                                          \
        vkDestroyImage(device, (img).image, NULL);        \
        vkFreeMemory(device, (img).memory, NULL);         \
    }

bool vulkan_create_image(uint32_t width, uint32_t height, uint32_t mipLevels,
    VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties, Image* image);

bool vulkan_create_image_3d(uint32_t width, uint32_t height, uint32_t depth, VkFormat format, VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties, Image* image);

bool vulkan_transition_image_layout(VkCommandPool commandPool, VkImage image,
    VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

bool vulkan_copy_buffer_to_image(VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

bool vulkan_copy_buffer_to_image_3d(VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth);

bool vulkan_create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageView* imageView);

bool vulkan_create_image_view_3d(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView);

bool vulkan_generate_mipmaps(VkCommandPool commandPool, VkImage image, VkFormat imageFormat,
    int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

#endif // IMAGE_H_
