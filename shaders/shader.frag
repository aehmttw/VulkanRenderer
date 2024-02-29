#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;


void main()
{
    outColor = fragColor * (dot(fragNormal, vec3(0.0, 0.0, 1.0)) / 2.0 + 0.5);
    outColor.a = fragColor.a;

    outColor = vec4(2.0 * outColor.rgb / (outColor.rgb + vec3(1, 1, 1)), outColor.a);
    outColor.rgb = vec3(min(outColor.r, 1.0), min(outColor.g, 1.0), min(outColor.b, 1.0));
}
