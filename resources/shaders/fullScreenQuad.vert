#version 450

layout(location = 0) out vec2 TexCoords;

const vec3 positions[6] = vec3[6] (
    vec3(-1.0,1.0,0.5),
    vec3(1.0,1.0,0.5),
    vec3(-1.0,-1.0,0.5),
    vec3(-1.0,-1.0,0.5),
    vec3(1.0,1.0,0.5),
    vec3(1.0,-1.0,0.5)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 1.0);
    TexCoords = ((gl_Position.xy) + vec2(1.)) * 0.5;
}