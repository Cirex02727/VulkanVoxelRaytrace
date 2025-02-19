#ifndef RAYTRACING_H_
#define RAYTRACING_H_

#include "vulkan_base.h"
#include "texture.h"

bool raytracing_init();

bool raytracing_add_volume_geometry(VkCommandPool commandPool, uint32_t width, uint32_t height, uint32_t depth, uint8_t* data);

void raytracing_add_triangle_geometry(VkBuffer vertexBuffer, VkBuffer indexBuffer,
    uint32_t vertexCount, uint64_t vertexStride, uint32_t indexCount, uint32_t material);

size_t raytracing_add_material(Material* material);

VkAccelerationStructureInstanceKHR* raytracing_add_volume_instance(uint32_t objIndex, VkTransformMatrixKHR* transform);
VkAccelerationStructureInstanceKHR* raytracing_add_triangle_instance(uint32_t objIndex, VkTransformMatrixKHR* transform);

void raytracing_update_instance(VkTransformMatrixKHR* transform, uint32_t instanceIndex);

bool raytracing_create_geometries_address_buffer(VkCommandPool commandPool);

bool raytracing_create_bottom_layer(VkCommandPool commandPool);
bool raytracing_create_top_layer(VkCommandPool commandPool);

bool raytracing_update_descriptor_sets(Images images, Images accumulateImages, Texture* texture);

bool raytracing_create_descriptors(Images images, Images accumulateImages, Texture* texture);
bool raytracing_create_pipeline(VkDescriptorSetLayout globalUBODescriptorSetLayout);
bool raytracing_create_shader_binding_table();

bool raytracer_render(VkCommandBuffer commandBuffer, VkImage accumulationImage, uint32_t frameIndex,
    uint32_t screenWidth, uint32_t screenHeight, VkDescriptorSet globalUBODescriptorSet);

void raytracing_clear_accumulation();

void raytracing_destroy();

#endif // RAYTRACING_H_
