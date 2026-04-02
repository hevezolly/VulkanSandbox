#version 450
layout(binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
} camera;

layout(binding = 1) uniform ModelUBO {
    mat4 transform;
} model;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 uv;

void main() {
    gl_Position = camera.projection * camera.view * model.transfrom * vec4(in_position, 1.0);
}