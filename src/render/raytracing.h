#ifndef RAYTRACING_H_
#define RAYTRACING_H_

#include "vulkan_base.h"
#include "texture.h"

bool raytracing_init();

void raytracing_add_volume_geometry(VkBuffer aabbBuffer, uint64_t stride, Image volume);

void raytracing_add_triangle_geometry(VkBuffer vertexBuffer, VkBuffer indexBuffer,
    uint32_t vertexCount, uint64_t vertexStride, uint32_t indexCount);

void raytracing_add_material(Material* material);

VkAccelerationStructureInstanceKHR* raytracing_add_volume_instance(uint32_t objIndex, VkTransformMatrixKHR* transform);
VkAccelerationStructureInstanceKHR* raytracing_add_triangle_instance(uint32_t objIndex, VkTransformMatrixKHR* transform);

void raytracing_update_instance(VkTransformMatrixKHR* transform, uint32_t instanceIndex);

bool raytracing_create_geometries_address_buffer(VkCommandPool commandPool);

bool raytracing_create_bottom_layer(VkCommandPool commandPool);
bool raytracing_create_top_layer(VkCommandPool commandPool);

bool raytracing_create_descriptors(Images images, Texture* texture);
bool raytracing_create_pipeline(VkDescriptorSetLayout globalUBODescriptorSetLayout);
bool raytracing_create_shader_binding_table();

bool raytracer_render(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t screenWidth, uint32_t screenHeight,
    VkDescriptorSet globalUBODescriptorSet);

void raytracing_destroy();

#endif // RAYTRACING_H_
