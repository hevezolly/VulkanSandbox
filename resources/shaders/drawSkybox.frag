#version 450
#extension GL_ARB_shading_language_include : enable
#include <commonData.glsl>

layout(location = 0) out vec4 FragColor;
layout(location = 0) in vec2 TexCoords;
  


layout(binding = 1) uniform samplerCube skybox;
layout(binding = 0) uniform CameraUBO {
    CameraData camera;
};

void main()
{             
    vec2 ndc = TexCoords * 2.0 - 1.0;

    // Vulkan NDC depth is [0, 1]. An interior depth also works
    // with reverse-Z and avoids an infinite far-plane endpoint.
    vec4 viewPoint = camera.invProjection * vec4(ndc, 0.5, 1.0);
    vec3 viewDir = viewPoint.xyz / viewPoint.w;

    // w = 0 excludes camera translation.
    vec3 worldDir = normalize(
        (camera.invView * vec4(viewDir, 0.0)).xyz
    );

    vec3 value = textureLod(skybox, worldDir, 0.0).rgb;

    FragColor = vec4(value, 1.0);
}