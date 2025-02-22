#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "rayShared.shinc"

layout(location = 0) rayPayloadInEXT vec3 payload;

void main()
{
    payload = vec3(0.18);
}
