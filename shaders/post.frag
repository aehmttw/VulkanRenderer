#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform sampler2D colorDepthSampler[2];


void main()
{
    outColor = texture(colorDepthSampler[0], fragTexCoord);
}
