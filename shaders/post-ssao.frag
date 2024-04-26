#version 450

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

layout(std430, set = 0, binding = 1) readonly buffer Samples
{
    int numberOfSamples;
    int samplesPerPixel;
    float sampleRadius;
    float strength;
    vec4 samples[ ];
};

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

    // Pseudo-random index generation using big primes
    int coordIndex = (int(gl_FragCoord.x) * 2725027 + int(gl_FragCoord.y)) * 6257561;
    vec3 random = samples[abs(coordIndex * 1163233) % numberOfSamples].xyz;
    vec3 tangent = normalize(random - normal * dot(random, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 tanBasis = mat3(tangent, bitangent, normal);

    // SSAO code adapted from https://learnopengl.com/Advanced-Lighting/SSAO
    float occludedSamples = 0;
    for (int i = 0; i < samplesPerPixel; i++)
    {
        int index = abs(6533647 * (coordIndex + i * 2181097)) % numberOfSamples;
        vec3 offset = -tanBasis * samples[index].xyz;
        offset.x *= -1.0;
        vec3 samplePos = offset * sampleRadius + pos;

        vec4 sampleScreenPos = ubo.proj * vec4(samplePos, 1);
        sampleScreenPos.xyz /= sampleScreenPos.w;
        sampleScreenPos.xy *= ubo.screenScale;
        sampleScreenPos.x *= -1.0;
        sampleScreenPos.xy = sampleScreenPos.xy * 0.5 + 0.5;

        float depthAtSample = texture(colorNormalSpecularDepthSampler[3], sampleScreenPos.xy).x;
        float d = ubo.screenDimensions.w / (depthAtSample - ubo.screenDimensions.z);

        float rangeFactor = smoothstep(0.0, 1.0, sampleRadius / abs(d - samplePos.z));

        if (sampleScreenPos.x >= 0.0 && sampleScreenPos.y >= 0.0 && sampleScreenPos.x <= 1.0 && sampleScreenPos.y <= 1.0 )
            occludedSamples += rangeFactor * ((d < samplePos.z - 0.01) ? 1 : 0);
    }

    outColor = texture(colorNormalSpecularDepthSampler[0], fragTexCoord.xy);
    outColor.rgb *= strength * vec3(1.0 - occludedSamples / float(samplesPerPixel)) + (1.0 - strength);
}
