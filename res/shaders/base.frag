#version 450 core

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

void main()
{
    outColor = fragColor * texture(texSampler, fragTexCoord * 2.0);
}
