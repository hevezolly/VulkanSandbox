#ifndef COMMON_DATA_GLSL_
#define COMMON_DATA_GLSL_

struct CameraData {
    mat4 view;
    mat4 projection;
    mat4 invView;
    mat4 invProjection;
    vec4 forward;
};

#endif