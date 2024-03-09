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

// 0 = Normal map; 1 = Albedo; 2 = Roughness; 3 = Metalness
layout(set = 1, binding = 0) uniform sampler2D texSampler[4];

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 albedo = texture(texSampler[1], fragTexCoord) * fragColor;
    float roughness = texture(texSampler[2], fragTexCoord).r;
    float metalness = texture(texSampler[3], fragTexCoord).r;

    float lod = roughness * 5;

    vec3 normal = normalize(fragTangentBasis * (2.0 * texture(texSampler[0], fragTexCoord).xyz - vec3(1, 1, 1)));
    vec3 dir = normalize(fragPosToCam);
    vec3 mirror = reflect(dir, normal);
    float cosAngle = abs(dot(dir, normal));

    // Schlick's approximation for index of refraction 1.5
    float ior = 1.5;
    float r0 = pow((1.0 - ior) / (1.0 + ior), 2.0);
    float schlick = r0 + (1 - r0) * pow(1 - cosAngle, 5);

    vec4 specular = vec4(textureLod(envTexSampler[0], normalize((ubo.env * vec4(mirror, 0)).xyz), lod).xyz, 1.0);
    vec4 metalColor = specular * albedo;
    vec4 dielectricColor = albedo + vec4(specular.xyz, 0) * schlick;

    outColor = metalColor * metalness + dielectricColor * (1.0 - metalness);

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
