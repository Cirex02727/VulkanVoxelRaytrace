#ifndef TEXTURE_H_
#define TEXTURE_H_

#include "vulkan_base.h"
#include "image.h"

typedef struct {
    Image image;

    VkSampler sampler;
    uint32_t mipLevels;
} Texture;

#define DeleteTexture(tex) {                               \
        if((tex).sampler)                                  \
            vkDestroySampler(device, (tex).sampler, NULL); \
                                                           \
        DeleteImage((tex).image);                          \
    }

bool vulkan_create_texture_image(const char* filepath, VkCommandPool commandPool,
    VkImageUsageFlags usage, bool mipmaps, Texture* texture);

bool vulkan_upload_texture_buffer_3d(void* data, uint32_t width, uint32_t height, uint32_t depth,
    VkCommandPool commandPool, Texture* texture);

bool vulkan_create_texture_image_view(Texture* texture);

bool vulkan_create_texture_sampler(Texture* texture);

#endif // TEXTURE_H_
