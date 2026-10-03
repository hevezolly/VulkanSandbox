#version 450
#extension GL_ARB_shading_language_include : enable
#include <brdf.glsl>
#include <commonData.glsl>

layout(binding = 0) uniform CameraUBO {
    CameraData camera;
};

layout(binding = 0, set=1) uniform TexturesUBO {
    vec4 f0roughness;
    float metallic;
    uint textureId;
};

layout(binding = 1, set = 1) uniform sampler2D textures[1];

layout(binding = 0, set = 2) uniform LightsData {

    mat4 DirShadowViewPorjection;
    vec4 DirLightDirection;
    vec4 DirLightColor;
    float DepthBias;

} Lights;

layout(binding = 1, set = 2) uniform sampler2D directShadowmap;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 position_world;
layout(location = 0) out vec4 outColor;

float SampleShadowmap(vec3 worldPosition) {
    vec4 shadowmapPos = (Lights.DirShadowViewPorjection * vec4(worldPosition, 1));
    shadowmapPos.xyz /= shadowmapPos.w;
    vec2 uv = (shadowmapPos.xy + vec2(1.0)) * 0.5;

    if (any(lessThan(uv, vec2(0.))) || any(greaterThan(uv, vec2(1.))))
        return 1;

    float fragmentDepth = shadowmapPos.z;
    float shadowmapDepth = texture(directShadowmap, uv).x;
    return float(fragmentDepth - Lights.DepthBias < shadowmapDepth);
}

void main() {
    
    const vec3 dielectric_f0 = vec3(0.04f);

    vec3 f0 = mix(dielectric_f0, f0roughness.xyz, metallic);


    vec3 n = normalize(in_normal);
    vec3 l = Lights.DirLightDirection.xyz;
    vec3 v = -camera.forward.xyz;

    vec3 f_factor = fresnel(f0, dot(n, v));
    vec3 kD = (vec3(1.0) - f_factor) * (1.0 - metallic);


    vec3 albedo = texture(textures[textureId], uv).xyz;

    vec3 diffuse = kD * albedo / PI;
    vec3 specular = specular_brdf(l, v, n, f0roughness.w, f0);

    float NdotL = max(dot(n, Lights.DirLightDirection.xyz), 0);
    vec3 light = Lights.DirLightColor.xyz * SampleShadowmap(position_world) * Lights.DirLightColor.w;

    vec3 color = (diffuse + specular) * light * NdotL;
    outColor = vec4(color, 1);

}