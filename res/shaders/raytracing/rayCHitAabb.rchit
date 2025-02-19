#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : require

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

// #extension GL_EXT_debug_printf : require

#include "rayShared.shinc"

hitAttributeEXT vec3 hitNormal;

layout(location = 0) rayPayloadInEXT Payload payload;
layout(location = 1) rayPayloadEXT bool isShadowed;

layout(set = 1, binding = 0) uniform accelerationStructureEXT as;

layout(set = 1, binding = 4, r8ui) uniform uimage3D volumes[];

struct Palette {
    vec3 color;
    uint material;
};

const Palette palette[] = Palette[] (
    Palette (vec3(1.0, 0.0, 1.0), 0),
    Palette (vec3(0.9, 0.9, 0.9), 0),
    Palette (vec3(0.8, 0.3, 0.2), 0),
    Palette (vec3(0.2, 0.8, 0.3), 0),
    Palette (vec3(0.3, 0.2, 0.8), 0),
    Palette (vec3(1.0, 0.0, 0.0), 1),
    Palette (vec3(0.0, 1.0, 0.0), 2),
    Palette (vec3(0.0, 0.0, 1.0), 3)
);

void main()
{
    // Debug Normals
    // payload = vec4(hitNormal * 0.5 + 0.5, 1.0);
    // return;
    
    // vec3 color = palette[imageLoad(volumes[gl_InstanceCustomIndexEXT], pos).r];

    vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + hitNormal * EPSILON;

    // vec3 lightDir = normalize(vec3(-1.0, -0.5, -2.0));

    // bool debug = all(lessThanEqual(gl_LaunchIDEXT.xy, vec2(0.0)));

    float attenuation = 1.0; // dot(hitNormal, lightDir);
    // if(attenuation > 0)
    // {
    //     uint flags  = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
 // 
    //     isShadowed = true;
    //     traceRayEXT(as,       // acceleration structure
    //                 flags,    // rayFlags
    //                 0xFF,     // cullMask
    //                 0,        // sbtRecordOffset
    //                 0,        // sbtRecordStride
    //                 1,        // missIndex
    //                 position, // ray origin
    //                 0.001,    // ray min range
    //                 lightDir, // ray direction
    //                 1000.0,   // ray max range
    //                 1         // payload (location = 1)
    //     );
    //     
    //     if(isShadowed)
    //         attenuation = 0.1;
    // }
    // else
    //     attenuation = 0.01;

    // if(debug)
    //     debugPrintfEXT("Start: %v3f | %f\n", hitNormal, attenuation);

    Palette plt = palette[gl_HitKindEXT];
    
    vec4 color = vec4(plt.color * attenuation, 1.0);

    payload.position = position;
    payload.normal = hitNormal;
    payload.color = color;
    payload.material = plt.material;
}
