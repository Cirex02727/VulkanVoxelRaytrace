#ifndef LIST_TYPES_H_
#define LIST_TYPES_H_

#include "core/list.h"
#include "core/vec.h"

#include <vulkan/vulkan.h>

#include <stddef.h>

typedef struct Image Image;

typedef struct BufferData BufferData;
typedef struct MappedBufferData MappedBufferData;

// ##### General #####

LIST_DEFINE(uint32_t, UInt32s);
LIST_DEFINE(uint8_t, UInt8s);

// ##### General #####

// ##### Vulkan #####

LIST_DEFINE(Image, Images);
LIST_DEFINE(VkFramebuffer, Framebuffers);
LIST_DEFINE(VkCommandBuffer, CommandBuffers);
LIST_DEFINE(VkSemaphore, Semaphores);
LIST_DEFINE(VkFence, Fences);
LIST_DEFINE(BufferData, BufferDatas);
LIST_DEFINE(MappedBufferData, MappedBufferDatas);
LIST_DEFINE(VkDescriptorSetLayout, DescriptorSetLayouts);
LIST_DEFINE(VkDescriptorSet, DescriptorSets);
LIST_DEFINE(VkFormat, Formats);

// ##### Vulkan #####

// ##### Raytracing Custome #####

typedef struct {
    Vec3 min, max;
    Vec2 padding;
} AABB;

typedef struct {
    Vec4 emission;
} Material;

LIST_DEFINE(AABB, AABBs);
LIST_DEFINE(VkDescriptorImageInfo, DescriptorImageInfos);

// ##### Raytracing Custome #####

#endif // LIST_TYPES_H_
