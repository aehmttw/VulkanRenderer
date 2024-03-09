#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in mat3 fragTangentBasis;
layout(location = 4) in vec2 fragTexCoord;
layout(location = 5) in vec3 fragPosToCam;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 proj;
    mat4 camera;
    mat4 env;
    vec4 cameraPos;
    bool hdr;
} ubo;

// 0 = PBR (5 mipmaps); 1 = Lambertian
layout(set = 0, binding = 1) uniform samplerCube envTexSampler[2];

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 normal = normalize(fragTangentBasis * (2.0 * texture(texSampler, fragTexCoord).xyz - vec3(1, 1, 1)));
    vec3 mirror = reflect(normalize(fragPosToCam), normal);
    outColor = vec4(texture(envTexSampler[0], normalize((ubo.env * vec4(mirror, 0)).xyz)).xyz, 1.0);

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
}
