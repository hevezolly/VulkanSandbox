#version 450

layout(binding = 0, set=1) uniform TexturesUBO {
    uint textureId;
};

layout(binding = 1, set = 1) uniform sampler2D textures[1];

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(textures[textureId], uv);
}