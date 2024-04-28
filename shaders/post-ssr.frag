#version 450

#define samples_per_ray 500

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 proj;
    mat4 camera;
    vec4 screenDimensions; // x and y: size of sensor at z=1, z and w: projectionA/B from website below
    vec2 screenScale; // scale to account for black bars at edge of screen
    bool hdr;
} ubo;

layout(set = 0, binding = 2) uniform sampler2D colorNormalSpecularDepthSampler[4];

// Coordinates recovering: adapted from https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
void main()
{
    float depthSampled = texture(colorNormalSpecularDepthSampler[3], fragTexCoord).x;

    vec2 viewCoord = (fragTexCoord * 2.0 - vec2(1.0)) * ubo.screenDimensions.xy;
    vec3 viewRay = (vec3(viewCoord, 1.0));
    float depth = ubo.screenDimensions.w / (depthSampled - ubo.screenDimensions.z);

    // Position in view space (before projection)
    vec3 pos = viewRay * depth;
    // Normal in view space
    vec3 normal = texture(colorNormalSpecularDepthSampler[1], fragTexCoord).xyz;
    normal.x = -normal.x;
    vec3 dir = reflect(normalize(-viewRay), normal);

    outColor = texture(colorNormalSpecularDepthSampler[0], fragTexCoord.xy);

    float specularSampled = texture(colorNormalSpecularDepthSampler[2], fragTexCoord).x;
    if (specularSampled <= 0.0)
        return;

    dir = -dir;
    float dirZ = max(dir.z, 0.0);
    vec3 normDir = dir;

    // We approximate fresnel reflectance because it conveniently makes reflections headed straight for
    // the camera dimmer. This is good because we don't know what's behind the camera.
    float schlick = 1.0 - pow(1.0 - dirZ, 5.0);

    dir /= length(dir.xy);

    // Step size of how much to move per each ray step for reflection calculation
    float step = 0.00002;

    // Normalize step direction to be uniform in fragment space
    {
        vec3 samplePos = dir * 0.01 + pos;
        vec4 sampleScreenPos = ubo.proj * vec4(samplePos, 1);
        sampleScreenPos.xyz /= sampleScreenPos.w;
        sampleScreenPos.xy *= ubo.screenScale;
        sampleScreenPos.x *= -1.0;
        sampleScreenPos.xy = sampleScreenPos.xy * 0.5 + 0.5;

        vec4 sampleScreenPos2 = ubo.proj * vec4(pos, 1);
        sampleScreenPos2.xyz /= sampleScreenPos2.w;
        sampleScreenPos2.xy *= ubo.screenScale;
        sampleScreenPos2.x *= -1.0;
        sampleScreenPos2.xy = sampleScreenPos2.xy * 0.5 + 0.5;

        dir *= 1.0 / max(length(sampleScreenPos2.xy - sampleScreenPos.xy), 0.001);
    }

    // March a ray until it hits something
    for (int i = 1; i <= samples_per_ray; i++)
    {
        vec3 samplePos = dir * i * step + pos;
        vec4 sampleScreenPos = ubo.proj * vec4(samplePos, 1);
        sampleScreenPos.xyz /= sampleScreenPos.w;
        sampleScreenPos.xy *= ubo.screenScale;
        sampleScreenPos.x *= -1.0;
        sampleScreenPos.xy = sampleScreenPos.xy * 0.5 + 0.5;

        float depthAtSample = texture(colorNormalSpecularDepthSampler[3], sampleScreenPos.xy).x;
        float d = ubo.screenDimensions.w / (depthAtSample - ubo.screenDimensions.z);

        // If a ray hits something
        if (d < samplePos.z - 0.025)
        {
            // Rays near the edge heading out of it have their reflectance dimmed to make screen-edge artifacts less jarring
            float dx = max((1.0 - sampleScreenPos.x) / normDir.x, (-sampleScreenPos.x) / normDir.x);
            float dy = max((1.0 - sampleScreenPos.y) / normDir.y, (-sampleScreenPos.y) / normDir.y);
            float a = min(dx * 5.0, 1.0);
            float b = min(dy * 5.0, 1.0);

            vec4 c = texture(colorNormalSpecularDepthSampler[0], sampleScreenPos.xy);
            float frac = schlick * (min(1.0, 2.0 * (1.0 - float(i) / float(samples_per_ray)))) * a * b * specularSampled;
            outColor = vec4(frac * c.xyz, 1.0) + (1.0 - frac) * outColor;
            break;
        }

        // If a ray goes offscreen
        if (!(sampleScreenPos.x >= 0.0 && sampleScreenPos.y >= 0.0 && sampleScreenPos.x <= 1.0 && sampleScreenPos.y <= 1.0))
            break;
    }
}
