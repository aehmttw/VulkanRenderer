#version 450
#define PI 3.1415926535
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec4 fragColor;
layout(location = 1) in mat3 fragTangentBasis;
layout(location = 4) in vec2 fragTexCoord;
layout(location = 5) in vec3 fragPosToCam;
layout(location = 6) in vec3 fragWorldPos;

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

// 0 = Normal map; 1 = Albedo; 2 = Specular
layout(set = 1, binding = 0) uniform sampler2D texSampler[3];

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outSpecular;

void main()
{
    vec3 normal = normalize(fragTangentBasis * (2.0 * texture(texSampler[0], fragTexCoord).xyz - vec3(1, 1, 1)));

    vec4 albedo = texture(texSampler[1], fragTexCoord) * fragColor;
    float specular = texture(texSampler[2], fragTexCoord).r;

    outColor = albedo;

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
    outSpecular = vec4(specular, specular, specular, specular);
}
