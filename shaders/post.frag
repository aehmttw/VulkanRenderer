#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform sampler2D colorNormalDepthSampler[3];


void main()
{
    outColor.xyz = texture(colorNormalDepthSampler[1], fragTexCoord).xyz / 2.0 + vec3(0.5);
    outColor.a = 1.0;
}
