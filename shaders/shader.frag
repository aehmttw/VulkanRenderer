#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;


void main()
{
    outColor = fragColor * (dot(fragNormal, vec3(0.0, 0.0, 1.0)) / 2.0 + 0.5);
    outColor.a = fragColor.a;
}
