#include "shader.h"

#include "core/filesystem.h"

#include <stdlib.h>

extern VkDevice device;

bool vulkan_shader_create_shader_module(uint32_t* code, size_t size, VkShaderModule* shader)
{
    VkShaderModuleCreateInfo createInfo = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode    = code,
    };

    VKCHECK(vkCreateShaderModule(device, &createInfo, NULL, shader));
    return true;
}

bool vulkan_shader_create_graphics_pipeline(const char* vertexShaderFilepath, const char* fragShaderFilepath,
    VkVertexInputBindingDescription vertexInputBindingDescription, VertexInputAttributeDescriptions vertexInputAttributeDescriptions,
    VkExtent2D swapchainSize, VkSampleCountFlagBits msaaSamples, VkDescriptorSetLayout* descriptorSetLayout,
    size_t descriptorSetLayoutCount, VkPipelineLayout* pipelineLayout, VkRenderPass renderPass, VkPipeline* graphicsPipeline)
{
    bool result = true;

    uint32_t *vertShaderCode = NULL, *fragShaderCode = NULL;
    size_t vertShaderSize, fragShaderSize;
    if(!file_read_all(vertexShaderFilepath, (char**)&vertShaderCode, &vertShaderSize))
    {
        log_error("Vulkan shader not found: %s", vertexShaderFilepath);
        finalize(false);
    }

    if(!file_read_all(fragShaderFilepath, (char**)&fragShaderCode, &fragShaderSize))
    {
        log_error("Vulkan shader not found: %s", fragShaderFilepath);
        finalize(false);
    }

    VkShaderModule vertShaderModule = 0;
    if(!vulkan_shader_create_shader_module(vertShaderCode, vertShaderSize, &vertShaderModule))
        finalize(false);
    
    VkShaderModule fragShaderModule = 0;
    if(!vulkan_shader_create_shader_module(fragShaderCode, fragShaderSize, &fragShaderModule))
        finalize(false);
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName  = "main",
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName  = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = ARRAYLEN(dynamicStates),
        .pDynamicStates    = dynamicStates,
    };
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &vertexInputBindingDescription,
        .vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.count,
        .pVertexAttributeDescriptions    = vertexInputAttributeDescriptions.items,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float) swapchainSize.width,
        .height   = (float) swapchainSize.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = swapchainSize,
    };
    
    VkPipelineViewportStateCreateInfo viewportState = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports    = &viewport,
        .scissorCount  = 1,
        .pScissors     = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .lineWidth               = 1.0f,
        .cullMode                = VK_CULL_MODE_BACK_BIT,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable  = VK_TRUE,
        .minSampleShading     = .2f,
        .rasterizationSamples = msaaSamples,
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable       = VK_TRUE,
        .depthWriteEnable      = VK_TRUE,
        .depthCompareOp        = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable     = VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable         = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable   = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments    = &colorBlendAttachment,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = descriptorSetLayoutCount,
        .pSetLayouts    = descriptorSetLayout,
    };

    if(vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, pipelineLayout) != VK_SUCCESS)
    {
        log_error("Vulkan vkcheck error");
        finalize(false);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = 2,
        .pStages             = shaderStages,
        .pVertexInputState   = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pDepthStencilState  = &depthStencil,
        .pColorBlendState    = &colorBlending,
        .pDynamicState       = &dynamicState,
        .layout              = *pipelineLayout,
        .renderPass          = renderPass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
    };

    if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, graphicsPipeline) != VK_SUCCESS)
    {
        log_error("Vulkan vkcheck error");
        finalize(false);
    }
    
finalize:
    if(vertShaderCode) free(vertShaderCode);
    if(fragShaderCode) free(fragShaderCode);

    if(vertShaderModule) vkDestroyShaderModule(device, fragShaderModule, NULL);
    if(fragShaderModule) vkDestroyShaderModule(device, vertShaderModule, NULL);
    return result;
}
