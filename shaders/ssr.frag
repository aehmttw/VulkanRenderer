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

// 0 = Normal map; 1 = Albedo; 2 = Roughness; 3 = Metalness
layout(set = 1, binding = 0) uniform sampler2D texSampler[4];

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;

// Vaguely adapted from https://learnopengl.com/PBR/Theory
float getReflectance(vec3 inVec, vec3 outVec, float roughness)
{
    float d = dot(inVec, outVec);

    // At really low values, floating point precision errors appear
    roughness = max(0.001, roughness);
    float r2 = roughness * roughness;

    float v = abs(d) * d * (r2 - 1.0f) + 1.0f;
    if (v < 0)
        v = 0;

    v = v * v * PI;

    if (v > 0)
        return d * r2 / v;

    return 0;
}

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

    // Use representative point from Epic Games:
    // https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
    vec3 additionalLight = vec3(0, 0, 0);
    for (int i = 0; i < lightsCount; i++)
    {
        vec3 reflectLightDir = normalize((lights[i].worldToLight * vec4(mirror, 0)).xyz);
        vec3 fragLightPoint = (lights[i].worldToLight * vec4(fragWorldPos, 1.0)).xyz;

        vec3 lightDir = -fragLightPoint;

        if (lights[i].isSun != 0)
            lightDir = vec3(0, 0, 1);

        // Closest point on ray to point:
        // https://stackoverflow.com/questions/73452295/find-the-closest-point-on-a-ray-from-anotherlook-point
        float d = dot(reflectLightDir, lightDir);
        if (d > 0) // If the light is behind us it can't light us up
        {
            vec3 closestPos = -lightDir + d * reflectLightDir;
            float distToCenter = length(closestPos);
            vec3 closestPosNorm = closestPos / distToCenter;

            float light;
            vec3 closestPosOnSphere = closestPos;

            if (distToCenter > lights[i].radius)
                closestPosOnSphere = closestPosNorm * lights[i].radius;

            vec3 outDir = normalize(closestPosOnSphere + lightDir);
            light = getReflectance(reflectLightDir, outDir, roughness);

            float intensity = 1.0;

            // = tan(angle of light) / 2
            float angleFactor;

            if (lights[i].isSun == 0)
            {
                float dist = max(length(fragLightPoint), lights[i].radius);
                intensity = max(0, 1.0 - pow(dist / lights[i].limit, 4)) * (1.0 / pow(dist, 2));
                angleFactor = lights[i].radius / (2.0 * dist);
            }
            else
                angleFactor = tan(lights[i].radius) / 2.0;

            // alpha and alpha' in Epic's notes - clamped at 0.01 to prevent it from getting too dark
            float a = pow(max(roughness, 0.1), 2.0);
            float a1 = a + angleFactor;
            // Normalization to account for conservation of power. See Epic Games paper linked above.
            intensity *= pow(a / a1, 2.0);

            if (lights[i].fov > 0)
            {
                vec3 dir = normalize(fragLightPoint);
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
                vec4 lightPos1 = lights[i].projection * vec4(fragLightPoint, 1.0);
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

    vec4 specular = vec4(textureLod(envTexSampler[0], normalize((ubo.env * vec4(mirror, 0)).xyz), lod).xyz + additionalLight, 1.0);
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

    outNormal = vec4(normalize((ubo.camera * vec4(normal, 0.0)).xyz), 1);
}
