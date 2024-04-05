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

// n + 4: nth shadow map
layout(set = 0, binding = 4) uniform sampler2D shadowMap[ ];

// 0 = Normal map; 1 = Albedo
layout(set = 1, binding = 0) uniform sampler2D texSampler[2];

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 normal = normalize(fragTangentBasis * (2.0 * texture(texSampler[0], fragTexCoord).xyz - vec3(1, 1, 1)));

    vec3 additionalLight = vec3(0, 0, 0);
    for (int i = 0; i < lightsCount; i++)
    {
        vec3 norm = normalize((lights[i].worldToLight * vec4(normal, 0)).xyz);

        if (lights[i].isSun != 0)
        {
            float light = max(0, norm.z + sin(lights[i].radius));
            additionalLight += light * lights[i].tintPower.rgb * lights[i].tintPower.a;
        }
        else
        {
            vec3 pos = (lights[i].worldToLight * vec4(fragWorldPos, 1.0)).xyz;
            vec3 dir = normalize(pos);
            float cosAngle = dot(dir, -norm);
            float light = max(0, cosAngle);
            float dist = max(length(pos), lights[i].radius);
            float intensity = max(0, 1.0 - pow(dist / lights[i].limit, 4)) * (1.0 / pow(dist, 2)) / (4.0 * PI);

            if (lights[i].fov > 0)
            {
                float cosLightAngle = -dir.z;
                float fovCosAngle = cos(lights[i].fov / 2.0);
                float fovCosAngleBlend = cos(lights[i].fov * (1.0 - lights[i].blend) / 2.0);
                float frac = max(min((fovCosAngleBlend - cosLightAngle) / (fovCosAngleBlend - fovCosAngle), 1.0), 0.0);
                intensity *= 1.0 - pow(frac, 2.0);
            }

            vec3 l = light * lights[i].tintPower.rgb * lights[i].tintPower.a * intensity;
            if (lights[i].shadowRes <= 0)
                additionalLight += l;
            else
            {
                vec4 lightPos1 = lights[i].projection * vec4(pos, 1.0);
                lightPos1.xyz /= lightPos1.w;
                lightPos1.xy = (lightPos1.xy / 2.0 + 0.5);
                float texSize = 1.0 / float(lights[i].shadowRes);

                float totalFrac = 0;
                for (int x = -1; x <= 1; x++)
                {
                    for (int y = -1; y <= 1; y++)
                    {
                        vec4 lightPos = lightPos1 + vec4(float(x) * texSize, float(y) * texSize, 0, 0);

                        float t1 = float(lightPos.z <= texture(shadowMap[i], lightPos.xy + vec2(-0.5 * texSize, -0.5 * texSize)).x);
                        float t2 = float(lightPos.z <= texture(shadowMap[i], lightPos.xy + vec2(0.5 * texSize, -0.5 * texSize)).x);
                        float t3 = float(lightPos.z <= texture(shadowMap[i], lightPos.xy + vec2(-0.5 * texSize, 0.5 * texSize)).x);
                        float t4 = float(lightPos.z <= texture(shadowMap[i], lightPos.xy + vec2(0.5 * texSize, 0.5 * texSize)).x);

                        vec2 coord = vec2(lightPos.xy / texSize) - 0.5;
                        vec2 frac2 = coord - vec2(float(int(coord.x)), float(int(coord.y)));
                        float frac = (t1 * (1.0 - frac2.x) + t2 * (frac2.x)) * (1.0 - frac2.y) + (t3 * (1.0 - frac2.x) + t4 * (frac2.x)) * frac2.y;

                        totalFrac += frac;
                    }
                }

                additionalLight += totalFrac / 9.0 * l;
            }
        }
    }

    vec4 albedo = texture(texSampler[1], fragTexCoord) * fragColor;
    vec3 envLight = texture(envTexSampler[1], normalize((ubo.env * vec4(normal, 0)).xyz)).xyz;
    outColor = vec4(albedo.rgb * (envLight + additionalLight), albedo.a);

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
