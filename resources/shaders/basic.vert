#version 450
#extension GL_ARB_shading_language_include : enable
#include <commonData.glsl>

layout(binding = 0) uniform CameraUBO {
    CameraData camera;
};

layout(binding = 1) uniform ModelUBO {
    mat4 transform;
    mat4 transformInv;
} model;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec2 uv_out;
layout(location = 1) out vec3 normal_out;
layout(location = 2) out vec3 position_world_out;

void main() {
    uv_out = uv;
    normal_out = normalize(mat3(transpose(model.transformInv))* in_normal);
    position_world_out = (model.transform * vec4(in_position, 1.0)).xyz;
    gl_Position = camera.projection * camera.view * vec4(position_world_out, 1);
    
}