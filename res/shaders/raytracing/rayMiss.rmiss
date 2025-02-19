#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "rayShared.shinc"

layout(location = 0) rayPayloadInEXT Payload payload;

void main()
{
    payload.position = vec3(0.0);
    payload.normal = vec3(0.0);
    
    payload.color = vec4(vec3(0.18), 0.0);

    payload.material = 0;
}
