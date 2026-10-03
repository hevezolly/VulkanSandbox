#ifndef UTILS_GLSL_
#define UTILS_GLSL_
#include <constants.glsl>

float radicalInverse(uint bits)
{
    return float(bitfieldReverse(bits)) * 2.3283064365386963e-10;
}

vec3 cubeTexelDirection(int face, ivec2 texel, int size)
{
    vec2 p = 2.0 * ((vec2(texel) + 0.5) / float(size)) - 1.0;

    vec3 d;
    switch (face) {
        case 0: d = vec3( 1.0, -p.y, -p.x); break;
        case 1: d = vec3(-1.0, -p.y,  p.x); break;
        case 2: d = vec3( p.x,  1.0,  p.y); break;
        case 3: d = vec3( p.x, -1.0, -p.y); break;
        case 4: d = vec3( p.x, -p.y,  1.0); break;
        case 5: d = vec3(-p.x, -p.y, -1.0); break;
        default: return vec3(0.0);
    }

    return normalize(d);
}

#endif