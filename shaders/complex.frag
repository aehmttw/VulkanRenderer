#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(binding = 0) uniform sampler2D texSampler[4];

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform UniformBufferObject
{
    mat4 model;
    mat4 proj;
} ubo;

vec3 toSkyBox(vec3 normal)
{
    vec3 n = vec3(abs(normal.x), abs(normal.y), abs(normal.z));
    if (n.x > n.y && n.x > n.z)
    {
        vec3 r = normal / n.x;
        int p = int(normal.x < 0.0);
        float ps = 2 * (float(p) - 0.5);
        return vec3(p, ps * r.z / 2.0 + 0.5, -r.y / 2.0 + 0.5);
    }
    else if (n.y > n.x && n.y > n.z)
    {
        vec3 r = normal / n.y;
        int p = int(normal.y < 0.0);
        float ps = 2 * (float(p) - 0.5);
        return vec3(p + 2, r.x / 2.0 + 0.5, -ps * r.z / 2.0 + 0.5);
    }
    else
    {
        vec3 r = normal / n.z;
        int p = int(normal.z < 0.0);
        float ps = 2 * (float(p) - 0.5);
        return vec3(p + 4, -ps * r.x / 2.0 + 0.5, -r.y / 2.0 + 0.5);
    }
}

void main()
{
    mat3 model3 = mat3(ubo.model[0].xyz, ubo.model[1].xyz, ubo.model[2].xyz);
    vec3 normal = normalize((model3 * fragNormal));

    //normal = reflect((ubo.proj * vec4(0, 0, 0, 1)).xyz, normal);

    outColor = fragColor * texture(texSampler[2], fragTexCoord);

    vec3 skyBoxCoord = toSkyBox(normal);
    vec2 texCoord = vec2(skyBoxCoord.y, skyBoxCoord / 6.0 + skyBoxCoord.z / 6.0);
    outColor.xyz = texture(texSampler[1], texCoord).xyz;

    outColor = vec4(2.0 * outColor.rgb / (outColor.rgb + vec3(1, 1, 1)), outColor.a);
    outColor.rgb = vec3(min(outColor.r, 1.0), min(outColor.g, 1.0), min(outColor.b, 1.0));
}
