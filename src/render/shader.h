#ifndef SHADER_H_
#define SHADER_H_

#include "vulkan_base.h"

typedef struct {
    VkVertexInputAttributeDescription* items;
    size_t                             count;
    size_t                             capacity;
} VertexInputAttributeDescriptions;

bool vulkan_shader_create_shader_module(uint32_t* code, size_t size, VkShaderModule* shader);

bool vulkan_shader_create_graphics_pipeline(const char* vertexShaderFilepath, const char* fragShaderFilepath,
    VkVertexInputBindingDescription vertexInputBindingDescription, VertexInputAttributeDescriptions vertexInputAttributeDescriptions,
    VkExtent2D swapchainSize, VkSampleCountFlagBits msaaSamples, VkDescriptorSetLayout* descriptorSetLayout,
    size_t descriptorSetLayoutCount, VkPipelineLayout* pipelineLayout, VkRenderPass renderPass, VkPipeline* graphicsPipeline);

#endif // SHADER_H_
