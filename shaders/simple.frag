#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outSpecular;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 proj;
    mat4 camera;
    mat4 env;
    vec4 cameraPos;
    bool hdr;
} ubo;

void main()
{
    vec3 normal = fragNormal;

    outColor = fragColor * (dot(normal, vec3(0.0, 0.0, 1.0)) / 4.0 + 0.75);
    outColor.a = fragColor.a;

    if (!ubo.hdr)
    {
        outColor = vec4(1.5 * outColor.rgb / (outColor.rgb + vec3(1, 1, 1)), outColor.a);
        outColor.rgb = vec3(min(outColor.r, 1.0), min(outColor.g, 1.0), min(outColor.b, 1.0));
    }
    else
    {
        //inspired by https://learnopengl.com/Advanced-Lighting/Gamma-Correction
        vec3 corrected_color = pow(outColor.rgb, vec3(1.0 / 2.2));
        outColor = vec4(corrected_color / (corrected_color + vec3(1, 1, 1)), outColor.a);
    }

    outNormal = vec4(normalize((ubo.camera * vec4(normal, 0.0)).xyz), 1);
    outSpecular = vec4(0);
}
