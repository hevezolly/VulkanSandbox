#version 450

layout(location = 0) out vec4 FragColor;
  
layout(location = 0) in vec2 TexCoords;

layout(binding = 0) uniform sampler2D img;
layout(binding = 1) uniform Ubo {
    float minValue;
    float maxValue;
} ubo;

void main()
{             
    float depthValue = texture(img, TexCoords).r;
    depthValue = smoothstep(ubo.minValue, ubo.maxValue, depthValue);
    FragColor = vec4(vec3(depthValue), 1.0);
}  