#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec4 color;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 proj;
    mat4 camera;
    vec4 cameraPos;
    bool hdr;
} ubo;

layout(push_constant) uniform PushConstant
{
    mat4 model;
} pc;

void main()
{
    gl_Position = vec4(position, 1.0);

    gl_Position = ubo.proj * ubo.camera * pc.model * vec4(position, 1.0);
    fragColor = color;
    fragNormal = norm;
}