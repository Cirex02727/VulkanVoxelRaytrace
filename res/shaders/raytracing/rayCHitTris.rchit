#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : require

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "rayShared.shinc"

hitAttributeEXT vec2 attribs;

struct Vertex
{
    vec4 pos;
    vec4 color;
    vec2 uv;

    vec2 padding;
};

layout(location = 0) rayPayloadInEXT Payload payload;
layout(location = 1) rayPayloadEXT bool isShadowed;

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices  { int    i[]; };

layout(set = 1, binding = 0) uniform accelerationStructureEXT as;

layout(set = 1, binding = 2, scalar) buffer GeometryDatas { GeometryData i[]; } geoDatas;
layout(set = 1, binding = 3) uniform sampler2D textureSampler;

// TODO: Pass via uniform the amount of aabbs for the objDesc offset [e.g. 1 aabb => gl_InstanceCustomIndexEXT - 1]

void main()
{
    // Object data
    GeometryData geoData  = geoDatas.i[gl_InstanceCustomIndexEXT - 1];
    Indices      indices  = Indices(geoData.indexAddress);
    Vertices     vertices = Vertices(geoData.vertexAddress);

    payload.material = geoData.material;
    
    // Indices of the triangle
    // ivec3 ind = indices.i[gl_PrimitiveID * 3];
    
    // Vertex of the triangle
    int offset = gl_PrimitiveID * 3;
    Vertex v0 = vertices.v[indices.i[offset + 0]];
    Vertex v1 = vertices.v[indices.i[offset + 1]];
    Vertex v2 = vertices.v[indices.i[offset + 2]];

    vec3 normal = normalize(cross(v1.pos.xyz - v0.pos.xyz, v2.pos.xyz - v0.pos.xyz));
    payload.normal = normal;

    vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + normal * EPSILON;
    payload.position = position;
    
    vec3 lightDir = normalize(vec3(0.0, 0.0, 1.0));

    float attenuation = dot(normal, lightDir);
    if(attenuation > 0)
    {
        uint flags  = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;

        isShadowed = true;
        traceRayEXT(as,       // acceleration structure
                    flags,    // rayFlags
                    0xFF,     // cullMask
                    0,        // sbtRecordOffset
                    0,        // sbtRecordStride
                    1,        // missIndex
                    position, // ray origin
                    0.001,    // ray min range
                    lightDir, // ray direction
                    1000.0,   // ray max range
                    1         // payload (location = 1)
        );

        if(isShadowed)
            attenuation = 0.1;
    }
    else
        attenuation = 0.01;

    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec2 texCoord = v0.uv * barycentrics.x + v1.uv * barycentrics.y + v2.uv * barycentrics.z;

    vec4 color = v0.color * barycentrics.x + v1.color * barycentrics.y + v2.color * barycentrics.z;
    
    color = color * texture(textureSampler, texCoord) * attenuation;
    
    payload.color = color;
}
