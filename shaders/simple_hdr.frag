#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout( push_constant ) uniform UniformBufferObject
{
    mat4 model;
    mat4 proj;
} ubo;

void main()
{
    vec3 normal = (ubo.model * vec4(fragNormal, 1)).xyz;

    outColor = fragColor * (dot(normal, vec3(0.0, 0.0, 1.0)) / 2.0 + 0.5);
    outColor.a = fragColor.a;

    //inspired by https://learnopengl.com/Advanced-Lighting/Gamma-Correction
    vec3 corrected_color = pow(outColor.rgb, vec3(1.0 / 2.2));
    outColor = vec4(corrected_color / (corrected_color + vec3(1, 1, 1)), outColor.a);
}
