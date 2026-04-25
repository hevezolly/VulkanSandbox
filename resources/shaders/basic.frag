#version 450

layout(binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
} camera;

layout(binding = 0, set=1) uniform TexturesUBO {
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
    vec3 normal = normalize(in_normal);

    float NdotL = max(dot(normal, Lights.DirLightDirection.xyz), 0);
    vec3 albedo = texture(textures[textureId], uv).xyz;

    vec3 light = NdotL * Lights.DirLightColor.xyz * SampleShadowmap(position_world);

    vec3 color = albedo * light;
    outColor = vec4(color, 1);

}