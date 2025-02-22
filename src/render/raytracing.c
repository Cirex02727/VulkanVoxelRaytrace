#include "raytracing.h"

#include "core/filesystem.h"
#include "core/camera.h"
#include "core/list.h"

#include "shader.h"
#include "buffer.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

extern VkDevice device;
extern VkPhysicalDevice physicalDevice;

extern uint32_t maxRayRecursionDepth;

typedef struct {
    VkBuffer        buffer;
    VkDeviceMemory  memory;
    VkDeviceAddress address;
} AddressedBuffer;

typedef struct {
    VkBuffer        buffer;
    VkDeviceMemory  memory;
    void*           map;
    VkDeviceAddress address;
} MappedAddressedBuffer;

typedef struct {
    VkDeviceAddress vertexAddress;
    VkDeviceAddress indexAddress;
} GeometryData;

typedef struct {
    VkAccelerationStructureGeometryKHR geometry;
    VkAccelerationStructureBuildRangeInfoKHR rangeInfo;
    VkBuildAccelerationStructureFlagsKHR structureFlags;
} BlasInput;

typedef struct {
    VkAccelerationStructureBuildGeometryInfoKHR geometryInfo;
    VkAccelerationStructureBuildRangeInfoKHR* rangeInfo;
    VkAccelerationStructureBuildSizesInfoKHR sizesInfo;
    VkAccelerationStructureKHR as;
} BuildAccelerationStructure;

typedef struct {
    BuildAccelerationStructure buildAs;
    VkDeviceAddress address;
} BottomLevel;

LIST_DEFINE(GeometryData, GeometriesAddresses);
LIST_DEFINE(BlasInput, BlasInputs);
LIST_DEFINE(BottomLevel, BottomLevels);
LIST_DEFINE(VkAccelerationStructureInstanceKHR, Tlas);
LIST_DEFINE(AddressedBuffer, AddressedBuffers);
LIST_DEFINE(BuildAccelerationStructure, BuildAccelerationStructures);

static PFN_vkGetBufferDeviceAddressKHR                   GetBufferDeviceAddressKHR                   = NULL;
static PFN_vkCreateAccelerationStructureKHR              CreateAccelerationStructureKHR              = NULL;
static PFN_vkCreateRayTracingPipelinesKHR                CreateRayTracingPipelinesKHR                = NULL;
static PFN_vkCmdBuildAccelerationStructuresKHR           CmdBuildAccelerationStructuresKHR           = NULL;
static PFN_vkGetAccelerationStructureBuildSizesKHR       GetAccelerationStructureBuildSizesKHR       = NULL;
static PFN_vkDestroyAccelerationStructureKHR             DestroyAccelerationStructureKHR             = NULL;
static PFN_vkGetRayTracingShaderGroupHandlesKHR          GetRayTracingShaderGroupHandlesKHR          = NULL;
static PFN_vkCmdTraceRaysKHR                             CmdTraceRaysKHR                             = NULL;
static PFN_vkGetAccelerationStructureDeviceAddressKHR    GetAccelerationStructureDeviceAddressKHR    = NULL;
static PFN_vkCmdWriteAccelerationStructuresPropertiesKHR CmdWriteAccelerationStructuresPropertiesKHR = NULL;

static BlasInputs       blasInputs = {0};
static BottomLevels     blass = {0};
static AddressedBuffers blasAddresses = {0};

static GeometriesAddresses geometriesAddresses = {0};
static BufferData          geometriesAddressesBuffer;

static BufferDatas aabbBuffers = {0};
static Images volumes = {0};

static Tlas                       tlas = {0};
static VkAccelerationStructureKHR tlasAs;
static MappedAddressedBuffer      instanceBuffer;

static BufferData accelerationBuffer;
static AddressedBuffer tempAsBuild;

static VkDescriptorSetLayout descriptorSetLayout;
static VkDescriptorPool descriptorPool;
static DescriptorSets descriptorSets;

static VkPipelineLayout pipelineLayout;
static VkPipeline pipeline;

static uint32_t sbtGroupCount;

static VkStridedDeviceAddressRegionKHR rayGenRegion;
static VkStridedDeviceAddressRegionKHR rayMissRegion;
static VkStridedDeviceAddressRegionKHR rayHitRegion;
static VkStridedDeviceAddressRegionKHR rayCallRegion;
static BufferData raySBTBuffer;

static VkDeviceAddress raytracing_get_buffer_device_address(VkBuffer buffer)
{
    VkBufferDeviceAddressInfo addressInfo =
    {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
        .buffer = buffer,
    };
    return GetBufferDeviceAddressKHR(device, &addressInfo);
}

bool raytracing_init()
{
    VK_DEVICE_PFN(device, GetBufferDeviceAddressKHR);
    VK_DEVICE_PFN(device, CreateAccelerationStructureKHR);
    VK_DEVICE_PFN(device, CreateRayTracingPipelinesKHR);
    VK_DEVICE_PFN(device, CmdBuildAccelerationStructuresKHR);
    VK_DEVICE_PFN(device, GetAccelerationStructureBuildSizesKHR);
    VK_DEVICE_PFN(device, DestroyAccelerationStructureKHR);
    VK_DEVICE_PFN(device, GetRayTracingShaderGroupHandlesKHR);
    VK_DEVICE_PFN(device, CmdTraceRaysKHR);
    VK_DEVICE_PFN(device, GetAccelerationStructureDeviceAddressKHR);
    VK_DEVICE_PFN(device, CmdWriteAccelerationStructuresPropertiesKHR);
    return true;
}

bool raytracing_add_volume_geometry(VkCommandPool commandPool, uint32_t width, uint32_t height, uint32_t depth, uint8_t* data)
{
    AABB aabb = {
        .min = { 0.0f, 0.0f, 0.0f },
        .max = { width, height, depth },
    };
    BufferData aabbBuffer;

    CHECK(vulkan_create_data_buffer(commandPool, &aabb, sizeof(AABB),
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, &aabbBuffer.buffer, &aabbBuffer.memory));
    
    Texture volume = {
        .mipLevels = 1,
    };

    CHECK(vulkan_create_image_3d(width, height, depth, VK_FORMAT_R8_UINT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &volume.image));
    CHECK(vulkan_create_image_view_3d(volume.image.image, VK_FORMAT_R8_UINT, VK_IMAGE_ASPECT_COLOR_BIT, &volume.image.view));

    CHECK(vulkan_upload_texture_buffer_3d(data, width, height, depth, commandPool, &volume));

    VkDeviceAddress address = raytracing_get_buffer_device_address(aabbBuffer.buffer);

    BlasInput blasInput = {
        .geometry = (VkAccelerationStructureGeometryKHR)
        {
            .sType          = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType   = VK_GEOMETRY_TYPE_AABBS_KHR,
            .geometry.aabbs = (VkAccelerationStructureGeometryAabbsDataKHR)
            {
                .sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,
                .data.deviceAddress = address,
                .stride             = sizeof(AABB),
            },
            .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
        },
        .rangeInfo = (VkAccelerationStructureBuildRangeInfoKHR)
        {
            .primitiveCount  = 1,
            .primitiveOffset = 0,
            .firstVertex     = 0,
            .transformOffset = 0,
        },
        .structureFlags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR,
    };
    list_append(blasInputs, blasInput);
    
    list_append(aabbBuffers, aabbBuffer);
    list_append(volumes, volume);
    return true;
}

void raytracing_add_triangle_geometry(VkBuffer vertexBuffer, VkBuffer indexBuffer,
    uint32_t vertexCount, uint64_t vertexStride, uint32_t indexCount)
{
    ASSERT(vertexCount > 0 && vertexStride > 0 && indexCount > 0);

    VkDeviceAddress vertexAddress = raytracing_get_buffer_device_address(vertexBuffer);
    VkDeviceAddress indexAddress  = raytracing_get_buffer_device_address(indexBuffer);

    VkAccelerationStructureGeometryTrianglesDataKHR triangles = {
        .sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
        .vertexFormat             = VK_FORMAT_R32G32B32_SFLOAT,
        .vertexData.deviceAddress = vertexAddress,
        .vertexStride             = vertexStride,
        .maxVertex                = vertexCount - 1,
        .indexType                = VK_INDEX_TYPE_UINT32,
        .indexData.deviceAddress  = indexAddress,
    };

    VkAccelerationStructureGeometryKHR geometry = {
        .sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType       = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry.triangles = triangles,
        .flags              = VK_GEOMETRY_OPAQUE_BIT_KHR
    };

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {
        .primitiveCount  = (uint32_t) (indexCount / 3),
        .primitiveOffset = 0,
        .firstVertex     = 0,
        .transformOffset = 0
    };

    BlasInput blasInput = {
        .geometry = geometry,
        .rangeInfo = rangeInfo,
        .structureFlags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR,
    };
    list_append(blasInputs, blasInput);

    GeometryData addresses = {
        .vertexAddress = vertexAddress,
        .indexAddress  = indexAddress,
    };
    list_append(geometriesAddresses, addresses);
}

VkAccelerationStructureInstanceKHR* raytracing_add_volume_instance(uint32_t objIndex, VkTransformMatrixKHR* transform)
{
    VkAccelerationStructureInstanceKHR instance = {
        .transform                              = *transform,
        .instanceCustomIndex                    = objIndex,
        .mask                                   = 0xFF,
        .instanceShaderBindingTableRecordOffset = 1,
        .flags                                  = VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR,
        .accelerationStructureReference         = blass.items[objIndex].address
    };
    return list_append(tlas, instance);
}

VkAccelerationStructureInstanceKHR* raytracing_add_triangle_instance(uint32_t objIndex, VkTransformMatrixKHR* transform)
{
    VkAccelerationStructureInstanceKHR instance = {
        .transform                              = *transform,
        .instanceCustomIndex                    = objIndex,
        .mask                                   = 0xFF,
        .instanceShaderBindingTableRecordOffset = 0,
        .flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
        .accelerationStructureReference         = blass.items[objIndex].address
    };
    return list_append(tlas, instance);
}

void raytracing_update_instance(VkTransformMatrixKHR* transform, uint32_t instanceIndex)
{
    memcpy(&tlas.items[instanceIndex].transform, transform, sizeof(VkTransformMatrixKHR));
}

bool raytracing_create_geometries_address_buffer(VkCommandPool commandPool)
{
    CHECK(vulkan_create_data_buffer(commandPool, geometriesAddresses.items, geometriesAddresses.count * sizeof(GeometryData),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0,
        &geometriesAddressesBuffer.buffer, &geometriesAddressesBuffer.memory));
    return true;
}

static bool raytracing_create_blas(VkCommandBuffer commandBuffer, UInt32s indices, BuildAccelerationStructures* buildAs,
    VkDeviceAddress scratchAddress, VkQueryPool queryPool)
{
    if (queryPool)
        vkResetQueryPool(device, queryPool, 0, (uint32_t) indices.count);
    
    uint32_t queryCnt = 0;

    blasAddresses.count = 0;
    for (size_t i = 0; i < indices.count; ++i)
    {
        uint32_t idx = indices.items[i];

        // Actual allocation of buffer and acceleration structure.
        BufferData accelerationBuffer;
        CHECK(vulkan_create_buffer(buildAs->items[idx].sizesInfo.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
            &accelerationBuffer.buffer, &accelerationBuffer.memory));
        
        VkAccelerationStructureCreateInfoKHR structureCreateInfo = {
            .sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .buffer = accelerationBuffer.buffer,
            .offset = 0,
            .size   = buildAs->items[idx].sizesInfo.accelerationStructureSize,
            .type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        };

        CreateAccelerationStructureKHR(device, &structureCreateInfo, NULL, &buildAs->items[idx].as);

        // BuildInfo #2 part
        buildAs->items[idx].geometryInfo.dstAccelerationStructure = buildAs->items[idx].as; // Setting where the build lands
        buildAs->items[idx].geometryInfo.scratchData.deviceAddress = scratchAddress;        // All build are using the same scratch buffer

        // Building the bottom-level-acceleration-structure
        CmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildAs->items[idx].geometryInfo,
            (const VkAccelerationStructureBuildRangeInfoKHR *const *)&buildAs->items[idx].rangeInfo);

        VkMemoryBarrier barrier = {
            .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
            .dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR
        };

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            0,
            1, &barrier,
            0, NULL,
            0, NULL);

        if(queryPool)
            CmdWriteAccelerationStructuresPropertiesKHR(commandBuffer, 1, &buildAs->items[idx].geometryInfo.dstAccelerationStructure,
                VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, queryCnt++);
        
        list_append(blasAddresses, accelerationBuffer);
    }
    return true;
}

static bool raytracing_create_bottom_level_as(VkCommandPool commandPool, VkBuildAccelerationStructureFlagsKHR flags)
{
    size_t nbBlas         = blasInputs.count;
    size_t nbCompactions  = 0;
    size_t maxScratchSize = 0;

    BuildAccelerationStructures buildAs = {0};
    list_alloc(buildAs, nbBlas);
    buildAs.count = nbBlas;

    for (uint32_t idx = 0; idx < nbBlas; idx++)
    {
        buildAs.items[idx] = (BuildAccelerationStructure) {
            .geometryInfo = (VkAccelerationStructureBuildGeometryInfoKHR) {
                .sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
                .type                     = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                .flags                    = blasInputs.items[idx].structureFlags | flags,
                .mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
                .srcAccelerationStructure = NULL,
                .dstAccelerationStructure = NULL,
                .geometryCount            = 1,
                .pGeometries              = &blasInputs.items[idx].geometry,
                .ppGeometries             = NULL,
            },
            .rangeInfo = &blasInputs.items[idx].rangeInfo,
            .sizesInfo = (VkAccelerationStructureBuildSizesInfoKHR) {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
            },
        };

        uint32_t maxPrimCount = blasInputs.items[idx].rangeInfo.primitiveCount;
        
        GetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &buildAs.items[idx].geometryInfo, &maxPrimCount, &buildAs.items[idx].sizesInfo);

        maxScratchSize = fmaxf(maxScratchSize, (uint32_t) buildAs.items[idx].sizesInfo.buildScratchSize);
        nbCompactions += buildAs.items[idx].geometryInfo.flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    }

    // Allocate the scratch buffers holding the temporary data of the acceleration structure builder
    BufferData scratchBuffer;
    CHECK(vulkan_create_buffer(maxScratchSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        &scratchBuffer.buffer, &scratchBuffer.memory));

    VkDeviceAddress scratchAddress = raytracing_get_buffer_device_address(scratchBuffer.buffer);

    // Allocate a query pool for storing the needed size for every BLAS compaction.
    VkQueryPool queryPool = NULL;
    if (nbCompactions > 0)
    {
        ASSERT(nbCompactions == nbBlas);
        VkQueryPoolCreateInfo qpci = {
            .sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
            .queryType          = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
            .queryCount         = nbBlas,
            .pipelineStatistics = 0
        };
        VKCHECK(vkCreateQueryPool(device, &qpci, NULL, &queryPool));
    }

    // Batching creation/compaction of BLAS to allow staying in restricted amount of memory
    UInt32s      indices    = {0};  // Indices of the BLAS to create
    VkDeviceSize batchSize  = 0;
    VkDeviceSize batchLimit = 256000000;  // 256 MB

    for (uint32_t idx = 0; idx < nbBlas; idx++)
    {
        list_append(indices, idx);
        batchSize += buildAs.items[idx].sizesInfo.accelerationStructureSize;

        // Over the limit or last BLAS element
        if (batchSize >= batchLimit || idx == nbBlas - 1)
        {
            VkCommandBuffer commandBuffer;
            CHECK(vulkan_begin_single_time_commands(commandPool, &commandBuffer));
            CHECK(raytracing_create_blas(commandBuffer, indices, &buildAs, scratchAddress, queryPool));
            CHECK(vulkan_end_single_time_commands(commandPool, commandBuffer));

            if (queryPool)
            {
                // VkCommandBuffer commandBuffer = VulkanApplication::BeginSingleTimeCommands(device, commandPool);
                // cmdCompactBlas(cmdBuf, indices, buildAs, queryPool);
                // VulkanApplication::EndSingleTimeCommands(device, m_Queue, commandPool, commandBuffer);
            }
            
            // Reset
            batchSize = 0;
            indices.count = 0;
        }
    }

    list_destroy(indices);

    // Keeping all the created acceleration structures
    VkAccelerationStructureDeviceAddressInfoKHR asDeviceAddressInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
    };

    for (size_t i = 0; i < buildAs.count; ++i)
    {
        asDeviceAddressInfo.accelerationStructure = buildAs.items[i].as;
        BottomLevel bottomLevel = {
            .buildAs = buildAs.items[i],
            .address = GetAccelerationStructureDeviceAddressKHR(device, &asDeviceAddressInfo),
        };
        list_append(blass, bottomLevel);
    }

    list_destroy(buildAs);

    // Clean up
    vkDestroyQueryPool(device, queryPool, NULL);

    DeleteBuffer(scratchBuffer);
    return true;
}

bool raytracing_create_bottom_layer(VkCommandPool commandPool)
{
    return raytracing_create_bottom_level_as(commandPool, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}

static bool raytracing_create_tlas(VkCommandBuffer commandBuffer,
    uint32_t countInstance, VkBuildAccelerationStructureFlagsKHR flags, bool update)
{
    // Wraps a device pointer to the above uploaded instances.
    VkAccelerationStructureGeometryInstancesDataKHR geometryInstancesData = {
        .sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
        .arrayOfPointers    = VK_FALSE,
        .data.deviceAddress = instanceBuffer.address,
    };

    // Put the above into a VkAccelerationStructureGeometryKHR. We need to put the instances struct in a union and label it as instance data.
    VkAccelerationStructureGeometryKHR topASGeometry = {
        .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry     = {},
        .flags        = 0,
    };
    topASGeometry.geometry.instances = geometryInstancesData;

    // Find sizes
    VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {
        .sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags                    = flags,
        .mode                     = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .srcAccelerationStructure = NULL,
        .dstAccelerationStructure = NULL,
        .geometryCount            = 1,
        .pGeometries              = &topASGeometry,
    };

    VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {
        .sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
        .accelerationStructureSize = 0,
        .updateScratchSize         = 0,
        .buildScratchSize          = 0,
    };
    GetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo,
        &countInstance, &sizeInfo);


    // Actual allocation of buffer and acceleration structure.
    if (!update)
    {
        CHECK(vulkan_create_buffer(sizeInfo.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
            &accelerationBuffer.buffer, &accelerationBuffer.memory));

        VkAccelerationStructureCreateInfoKHR createInfo = {
            .sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .createFlags   = 0,
            .buffer        = accelerationBuffer.buffer,
            .offset        = 0,
            .size          = sizeInfo.accelerationStructureSize,
            .type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
            .deviceAddress = 0,
        };

        VKCHECK(CreateAccelerationStructureKHR(device, &createInfo, NULL, &tlasAs));

        // Allocate the scratch buffers holding the temporary data of the acceleration structure builder
        CHECK(vulkan_create_buffer(sizeInfo.buildScratchSize,
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
            &tempAsBuild.buffer, &tempAsBuild.memory));

        tempAsBuild.address = raytracing_get_buffer_device_address(tempAsBuild.buffer);
    }

    // Update build information
    buildInfo.srcAccelerationStructure = update ? tlasAs : NULL;
    buildInfo.dstAccelerationStructure = tlasAs;
    buildInfo.scratchData.deviceAddress = tempAsBuild.address;

    // Build Offsets info: n instances
    VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo = {
        .primitiveCount  = countInstance,
        .primitiveOffset = 0,
        .firstVertex     = 0,
        .transformOffset = 0
    };
    const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

    // Build the TLAS
    CmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo, &pBuildOffsetInfo);
    return true;
}

static bool raytracing_build_tlas(VkCommandPool commandPool, VkBuildAccelerationStructureFlagsKHR flags, bool update)
{
    size_t countInstance = tlas.count;
    size_t sizeInstance = countInstance * sizeof(VkAccelerationStructureInstanceKHR);

    if (!update)
    {
        CHECK(vulkan_create_mapped_data_buffer(tlas.items, sizeInstance,
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
            &instanceBuffer.buffer, &instanceBuffer.memory, &instanceBuffer.map));
        
        instanceBuffer.address = raytracing_get_buffer_device_address(instanceBuffer.buffer);
    }
    else
        memcpy(instanceBuffer.map, tlas.items, sizeInstance);
    
    VkCommandBuffer commandBuffer;
    CHECK(vulkan_begin_single_time_commands(commandPool, &commandBuffer));

    VkMemoryBarrier barrier = {
        .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR
    };

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        0,
        1, &barrier,
        0, NULL,
        0, NULL);

    // Creating the TLAS
    CHECK(raytracing_create_tlas(commandBuffer, countInstance, flags, update));
    
    CHECK(vulkan_end_single_time_commands(commandPool, commandBuffer));
    return true;
}

bool raytracing_create_top_layer(VkCommandPool commandPool)
{
    return raytracing_build_tlas(commandPool,
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR, false);
}

bool raytracing_update_top_layer(VkCommandPool commandPool)
{
    return raytracing_build_tlas(commandPool,
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR, true);
}

static bool raytracing_create_descriptor_set_layouts()
{
    VkDescriptorSetLayoutBinding asLayoutBinding = {
        .binding            = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
        .descriptorCount    = 1,
        .stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
    };

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {
        .binding            = 1,
        .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount    = 1,
        .stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
    };

    VkDescriptorSetLayoutBinding objsLayoutBinding = {
        .binding            = 2,
        .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount    = 1,
        .stageFlags         = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
    };

    VkDescriptorSetLayoutBinding texturesLayoutBinding = {
        .binding            = 3,
        .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount    = 1,
        .stageFlags         = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
    };

    VkDescriptorSetLayoutBinding volumesLayoutBinding = {
        .binding            = 4,
        .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount    = volumes.count,
        .stageFlags         = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
    };
    
    VkDescriptorSetLayoutBinding bindings[] = {
        asLayoutBinding, samplerLayoutBinding, objsLayoutBinding, texturesLayoutBinding, volumesLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .flags        = 0,
        .bindingCount = ARRAYLEN(bindings),
        .pBindings    = bindings,
    };

    VKCHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &descriptorSetLayout));
    return true;
}

bool raytracing_update_descriptor_sets(Images images, Texture* texture)
{
    VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureInfo = {
        .sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount = 1,
        .pAccelerationStructures    = &tlasAs,
    };

    VkDescriptorBufferInfo objsInfo = {
        .buffer = geometriesAddressesBuffer.buffer,
        .offset = 0,
        .range  = geometriesAddresses.count * sizeof(GeometryData),
    };

    VkDescriptorImageInfo textureInfo = {
        .sampler     = texture->sampler,
        .imageView   = texture->image.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    DescriptorImageInfos volumesInfos = {0};
    list_alloc(volumesInfos, volumes.count);

    for(size_t i = 0; i < volumes.count; ++i)
    {
        VkDescriptorImageInfo info = {
            .sampler = NULL,
            .imageView = volumes.items[i].view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        list_append(volumesInfos, info);
    }

    for (size_t i = 0; i < images.count; ++i)
    {
        VkDescriptorImageInfo imageInfo = {
            .sampler     = NULL,
            .imageView   = images.items[i].view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        
        VkWriteDescriptorSet descriptorWrites[] = {
            (VkWriteDescriptorSet) {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext            = &accelerationStructureInfo,
                .dstSet           = descriptorSets.items[i],
                .dstBinding       = 0,
                .dstArrayElement  = 0,
                .descriptorCount  = 1,
                .descriptorType   = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            },
            (VkWriteDescriptorSet) {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = descriptorSets.items[i],
                .dstBinding       = 1,
                .dstArrayElement  = 0,
                .descriptorCount  = 1,
                .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo       = &imageInfo,
            },
            (VkWriteDescriptorSet) {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = descriptorSets.items[i],
                .dstBinding       = 2,
                .dstArrayElement  = 0,
                .descriptorCount  = 1,
                .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo      = &objsInfo,
            },
            (VkWriteDescriptorSet) {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = descriptorSets.items[i],
                .dstBinding       = 3,
                .dstArrayElement  = 0,
                .descriptorCount  = 1,
                .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo       = &textureInfo,
            },
            (VkWriteDescriptorSet) {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = descriptorSets.items[i],
                .dstBinding       = 4,
                .dstArrayElement  = 0,
                .descriptorCount  = volumesInfos.count,
                .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo       = volumesInfos.items,
            },
        };

        vkUpdateDescriptorSets(device, ARRAYLEN(descriptorWrites), descriptorWrites, 0, NULL);
    }
    return true;
}

bool raytracing_create_descriptors(Images images, Texture* texture)
{
    CHECK(raytracing_create_descriptor_set_layouts());

    VkDescriptorPoolSize poolSizes[] = {
        (VkDescriptorPoolSize) {
            .type            = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = images.count,
        },
        (VkDescriptorPoolSize) {
            .type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = images.count,
        },
        (VkDescriptorPoolSize) {
            .type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = images.count,
        },
        (VkDescriptorPoolSize) {
            .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = images.count,
        },
        (VkDescriptorPoolSize) {
            .type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = images.count,
        },
    };

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = images.count * ARRAYLEN(poolSizes),
        .poolSizeCount = ARRAYLEN(poolSizes),
        .pPoolSizes    = poolSizes,
    };

    VKCHECK(vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptorPool));

    DescriptorSetLayouts layouts = {0};
    list_alloc(layouts, images.count);

    for(size_t i = 0; i < images.count; ++i)
        list_append(layouts, descriptorSetLayout);
    
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = descriptorPool,
        .descriptorSetCount = layouts.count,
        .pSetLayouts        = layouts.items,
    };

    list_alloc(descriptorSets, images.count);
    descriptorSets.count = images.count;
    
    VKCHECK(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.items));

    CHECK(raytracing_update_descriptor_sets(images, texture));
    return true;
}

bool raytracing_create_pipeline(VkDescriptorSetLayout globalUBODescriptorSetLayout)
{
    enum Shaders {
        rGen, rMiss, rShadow, rCHitTris, rCHitAabb, rIntAabb,
    };

    bool result = true;

    const char* rgenShaderFilepath      = "res/shaders/raytracing/rayGen.rgen.spv";
    const char* rmissShaderFilepath     = "res/shaders/raytracing/rayMiss.rmiss.spv";
    const char* rshadowShaderFilepath   = "res/shaders/raytracing/rayShadow.rmiss.spv";
    const char* rchitTrisShaderFilepath = "res/shaders/raytracing/rayCHitTris.rchit.spv";
    const char* rchitAabbShaderFilepath = "res/shaders/raytracing/rayCHitAabb.rchit.spv";
    const char* rintAabbShaderFilepath = "res/shaders/raytracing/rayIntAabb.rint.spv";

    VkDescriptorSetLayout descriptorSetLayouts[] = {
        globalUBODescriptorSetLayout,
        descriptorSetLayout,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .flags          = VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT,
        .setLayoutCount = ARRAYLEN(descriptorSetLayouts),
        .pSetLayouts    = descriptorSetLayouts,
    };

    VKCHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout));

    uint32_t *rgenShaderCode = NULL, *rmissShaderCode = NULL, *rshadowShaderCode = NULL,
             *rchitTrisShaderCode = NULL, *rchitAabbShaderCode = NULL, *rintAabbShaderCode = NULL;
    size_t rgenShaderCodeSize, rmissShaderCodeSize, rshadowShaderCodeSize,
           rchitTrisShaderCodeSize, rchitAabbShaderCodeSize, rintAabbShaderCodeSize;

    if(!file_read_all(rgenShaderFilepath, (char**)&rgenShaderCode, &rgenShaderCodeSize))
    {
        log_error("Vulkan shader not found: %s", rgenShaderFilepath);
        finalize(false);
    }

    VkShaderModule rgenShaderModule = 0;
    if(!vulkan_shader_create_shader_module(rgenShaderCode, rgenShaderCodeSize, &rgenShaderModule))
        finalize(false);

    if(!file_read_all(rmissShaderFilepath, (char**)&rmissShaderCode, &rmissShaderCodeSize))
    {
        log_error("Vulkan shader not found: %s", rmissShaderFilepath);
        finalize(false);
    }

    VkShaderModule rmissShaderModule = 0;
    if(!vulkan_shader_create_shader_module(rmissShaderCode, rmissShaderCodeSize, &rmissShaderModule))
        finalize(false);

    if(!file_read_all(rshadowShaderFilepath, (char**)&rshadowShaderCode, &rshadowShaderCodeSize))
    {
        log_error("Vulkan shader not found: %s", rshadowShaderFilepath);
        finalize(false);
    }

    VkShaderModule rshadowShaderModule = 0;
    if(!vulkan_shader_create_shader_module(rshadowShaderCode, rshadowShaderCodeSize, &rshadowShaderModule))
        finalize(false);

    if(!file_read_all(rchitTrisShaderFilepath, (char**)&rchitTrisShaderCode, &rchitTrisShaderCodeSize))
    {
        log_error("Vulkan shader not found: %s", rchitTrisShaderFilepath);
        finalize(false);
    }

    VkShaderModule rchitTrisShaderModule = 0;
    if(!vulkan_shader_create_shader_module(rchitTrisShaderCode, rchitTrisShaderCodeSize, &rchitTrisShaderModule))
        finalize(false);

    if(!file_read_all(rchitAabbShaderFilepath, (char**)&rchitAabbShaderCode, &rchitAabbShaderCodeSize))
    {
        log_error("Vulkan shader not found: %s", rchitAabbShaderFilepath);
        finalize(false);
    }

    VkShaderModule rchitAabbShaderModule = 0;
    if(!vulkan_shader_create_shader_module(rchitAabbShaderCode, rchitAabbShaderCodeSize, &rchitAabbShaderModule))
        finalize(false);

    if(!file_read_all(rintAabbShaderFilepath, (char**)&rintAabbShaderCode, &rintAabbShaderCodeSize))
    {
        log_error("Vulkan shader not found: %s", rintAabbShaderFilepath);
        finalize(false);
    }

    VkShaderModule rintAabbShaderModule = 0;
    if(!vulkan_shader_create_shader_module(rintAabbShaderCode, rintAabbShaderCodeSize, &rintAabbShaderModule))
        finalize(false);
    
    VkPipelineShaderStageCreateInfo rgenShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        .module = rgenShaderModule,
        .pName  = "main",
    };

    VkPipelineShaderStageCreateInfo rmissShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_MISS_BIT_KHR,
        .module = rmissShaderModule,
        .pName  = "main",
    };

    VkPipelineShaderStageCreateInfo rshadowShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_MISS_BIT_KHR,
        .module = rshadowShaderModule,
        .pName  = "main",
    };

    VkPipelineShaderStageCreateInfo rchitTrisShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        .module = rchitTrisShaderModule,
        .pName  = "main",
    };

    VkPipelineShaderStageCreateInfo rchitAabbShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        .module = rchitAabbShaderModule,
        .pName  = "main",
    };

    VkPipelineShaderStageCreateInfo rintAabbShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
        .module = rintAabbShaderModule,
        .pName  = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        rgenShaderStageInfo, rmissShaderStageInfo, rshadowShaderStageInfo,
        rchitTrisShaderStageInfo, rchitAabbShaderStageInfo, rintAabbShaderStageInfo,
    };

    VkRayTracingShaderGroupCreateInfoKHR rayGenGroup = {
        .sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        .generalShader      = rGen,
        .closestHitShader   = VK_SHADER_UNUSED_KHR,
        .anyHitShader       = VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR,
    };

    VkRayTracingShaderGroupCreateInfoKHR rayMissGroup =
    {
        .sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        .generalShader      = rMiss,
        .closestHitShader   = VK_SHADER_UNUSED_KHR,
        .anyHitShader       = VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR,
    };

    VkRayTracingShaderGroupCreateInfoKHR rayShadowGroup =
    {
        .sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        .generalShader      = rShadow,
        .closestHitShader   = VK_SHADER_UNUSED_KHR,
        .anyHitShader       = VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR,
    };

    VkRayTracingShaderGroupCreateInfoKHR rayHitTrisGroup =
    {
        .sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
        .generalShader      = VK_SHADER_UNUSED_KHR,
        .closestHitShader   = rCHitTris,
        .anyHitShader       = VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR,
    };

    VkRayTracingShaderGroupCreateInfoKHR rayHitAabbGroup =
    {
        .sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR,
        .generalShader      = VK_SHADER_UNUSED_KHR,
        .closestHitShader   = rCHitAabb,
        .anyHitShader       = VK_SHADER_UNUSED_KHR,
        .intersectionShader = rIntAabb,
    };

    VkRayTracingShaderGroupCreateInfoKHR shaderGroups[] = {
        rayGenGroup, rayMissGroup, rayShadowGroup, rayHitTrisGroup, rayHitAabbGroup,
    };

    VkRayTracingPipelineCreateInfoKHR pipelineInfo = {
        .sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .stageCount                   = ARRAYLEN(shaderStages),
        .pStages                      = shaderStages,
        .groupCount                   = ARRAYLEN(shaderGroups),
        .pGroups                      = shaderGroups,
        .maxPipelineRayRecursionDepth = maxRayRecursionDepth,
        .pLibraryInfo                 = NULL,
        .pLibraryInterface            = NULL,
        .pDynamicState                = NULL,
        .layout                       = pipelineLayout,
        .basePipelineHandle           = NULL,
        .basePipelineIndex            = 0,
    };

    sbtGroupCount = pipelineInfo.groupCount;

    VKCHECK(CreateRayTracingPipelinesKHR(device, NULL, NULL, 1, &pipelineInfo, NULL, &pipeline));
    
finalize:
    if(rgenShaderCode) free(rgenShaderCode);
    if(rmissShaderCode) free(rmissShaderCode);
    if(rshadowShaderCode) free(rshadowShaderCode);
    if(rchitTrisShaderCode) free(rchitTrisShaderCode);
    if(rchitAabbShaderCode) free(rchitAabbShaderCode);
    if(rintAabbShaderCode) free(rintAabbShaderCode);
    
    if(rgenShaderModule) vkDestroyShaderModule(device, rgenShaderModule, NULL);
    if(rmissShaderModule) vkDestroyShaderModule(device, rmissShaderModule, NULL);
    if(rshadowShaderModule) vkDestroyShaderModule(device, rshadowShaderModule, NULL);
    if(rchitTrisShaderModule) vkDestroyShaderModule(device, rchitTrisShaderModule, NULL);
    if(rchitAabbShaderModule) vkDestroyShaderModule(device, rchitAabbShaderModule, NULL);
    if(rintAabbShaderModule) vkDestroyShaderModule(device, rintAabbShaderModule, NULL);
    return result;
}

static uint32_t raytracing_align_up(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

bool raytracing_create_shader_binding_table()
{
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
    };
    VkPhysicalDeviceProperties2 deviceProperties2 =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &rayTracingPipelineProperties,
    };
    vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);

    uint32_t missCount = 2;
    uint32_t hitCount  = 2;
    uint32_t handleCount = 1 + missCount + hitCount;
    uint32_t handleSize  = rayTracingPipelineProperties.shaderGroupHandleSize;

    uint32_t handleSizeAligned = raytracing_align_up(handleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);

    rayGenRegion.stride  = raytracing_align_up(handleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment);
    rayGenRegion.size    = rayGenRegion.stride;
    rayMissRegion.stride = handleSizeAligned;
    rayMissRegion.size   = raytracing_align_up(missCount * handleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment);
    rayHitRegion.stride  = handleSizeAligned;
    rayHitRegion.size    = raytracing_align_up(hitCount * handleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment);

    rayCallRegion = (VkStridedDeviceAddressRegionKHR) {0};

    uint32_t dataSize = handleCount * handleSize;
    UInt8s handles = {0};
    list_alloc(handles, dataSize);
    handles.count = dataSize;

    VKCHECK(GetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, handleCount, dataSize, handles.items));

    VkDeviceSize sbtSize = rayGenRegion.size + rayMissRegion.size + rayHitRegion.size + rayCallRegion.size;
    CHECK(vulkan_create_buffer(sbtSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, &raySBTBuffer.buffer, &raySBTBuffer.memory));
    
    VkDeviceAddress sbtAddress  = raytracing_get_buffer_device_address(raySBTBuffer.buffer);
    rayGenRegion.deviceAddress  = sbtAddress;
    rayMissRegion.deviceAddress = sbtAddress + rayGenRegion.size;
    rayHitRegion.deviceAddress  = sbtAddress + rayGenRegion.size + rayMissRegion.size;

    void* pSBTBuffer;
    VKCHECK(vkMapMemory(device, raySBTBuffer.memory, 0, sbtSize, 0, &pSBTBuffer));

    // Raygen
    memcpy(pSBTBuffer, handles.items, handleSize);

    uint32_t handleIdx = 1;

    // Miss
    uint8_t* pData = pSBTBuffer + rayGenRegion.size;
    for(size_t i = 0; i < missCount; ++i)
    {
        memcpy(pData, handles.items + (handleIdx++) * handleSize, handleSize);
        pData += rayMissRegion.stride;
    }

    // Hit
    pData = pSBTBuffer + rayGenRegion.size + rayMissRegion.size;
    for(size_t i = 0; i < hitCount; ++i)
    {
        memcpy(pData, handles.items + (handleIdx++) * handleSize, handleSize);
        pData += rayHitRegion.stride;
    }

    vkUnmapMemory(device, raySBTBuffer.memory);
    return true;
}

bool raytracer_render(VkCommandBuffer commandBuffer, uint32_t frameIndex,
    uint32_t screenWidth, uint32_t screenHeight, VkDescriptorSet globalUBODescriptorSet)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout,
        0, 1, &globalUBODescriptorSet,           0, 0);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout,
        1, 1, &descriptorSets.items[frameIndex], 0, 0);
    
    CmdTraceRaysKHR(commandBuffer, &rayGenRegion, &rayMissRegion, &rayHitRegion, &rayCallRegion,
        screenWidth, screenHeight, 1);
    return true;
}

void raytracing_destroy()
{
    for(size_t i = 0; i < aabbBuffers.count; ++i)
        DeleteBuffer(aabbBuffers.items[i]);
    
    for(size_t i = 0; i < volumes.count; ++i)
        DeleteImage(volumes.items[i]);

    DeleteBuffer(raySBTBuffer);

    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyPipelineLayout(device, pipelineLayout, NULL);

    vkFreeDescriptorSets(device, descriptorPool, descriptorSets.count, descriptorSets.items);

    vkDestroyDescriptorPool(device, descriptorPool, NULL);

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);

    DeleteBuffer(geometriesAddressesBuffer);

    // Bottom layer
    for(size_t i = 0; i < blasAddresses.count; ++i)
        DeleteBuffer(blasAddresses.items[i]);

    for(size_t i = 0; i < blass.count; ++i)
        DestroyAccelerationStructureKHR(device, blass.items[i].buildAs.as, NULL);
    
    // Top layer
    DeleteBuffer(tempAsBuild);
    DeleteBuffer(accelerationBuffer);
    DeleteMappedBuffer(instanceBuffer);

    DestroyAccelerationStructureKHR(device, tlasAs, NULL);
}
