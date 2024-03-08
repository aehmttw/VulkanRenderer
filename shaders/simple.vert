#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec4 color;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;

layout( push_constant ) uniform UniformBufferObject
{
    mat4 model;
    mat4 proj;
} ubo;

void main()
{
    gl_Position = vec4(position, 1.0);

    gl_Position = ubo.proj * ubo.model * vec4(position, 1.0);
    fragColor = color;
    fragNormal = norm;
}