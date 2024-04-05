#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 tangent;
layout(location = 3) in vec2 texCoord;
layout(location = 4) in vec4 color;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out mat3 fragTangentBasis;
layout(location = 4) out vec2 fragTexCoord;
layout(location = 5) out vec3 fragPosToCam;
layout(location = 6) out vec3 fragWorldPos;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 proj;
    mat4 camera;
    mat4 env;
    vec4 cameraPos;
    bool hdr;
} ubo;

struct Light
{
    mat4 worldToLight;
    mat4 projection;
    vec4 tintPower;
    float radius;
    float limit;
    float fov;
    float blend;
    int isSun;
    int shadowRes;
};

layout(std430, set = 0, binding = 3) readonly buffer Lights
{
    int lightsCount;
    Light lights[ ];
};

layout(push_constant) uniform PushConstant
{
    mat4 model;
    int shadowMap;
} pc;

void main()
{
    gl_Position = vec4(position, 1.0);

    fragWorldPos = (pc.model * vec4(position, 1.0)).xyz;
    fragPosToCam = fragWorldPos - ubo.cameraPos.xyz;

    if (pc.shadowMap >= 0)
        gl_Position = lights[pc.shadowMap].projection * lights[pc.shadowMap].worldToLight * vec4(fragWorldPos, 1.0);
    else
        gl_Position = ubo.proj * ubo.camera * vec4(fragWorldPos, 1.0);

    fragColor = color;
    fragTexCoord = texCoord;

    // Adapted from https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    vec3 tan = normalize(vec3(pc.model * vec4(tangent.xyz, 0.0)));
    vec3 bitan = normalize(vec3(pc.model * vec4(cross(tangent.xyz, normal) * tangent.w, 0.0)));
    vec3 norm = normalize(vec3(pc.model * vec4(normal, 0.0)));
    fragTangentBasis = mat3(tan, bitan, norm);
}