#ifndef BRDF_GLSL_
#define BRDF_GLSL_
#include <constants.glsl>

// beckmann brdf for specular, lambertian for diffuse

vec3 fresnel(vec3 f0, float ndotl) {
    float negndotl = 1.0f - ndotl;
    return f0 + (1 - f0) * negndotl * negndotl * negndotl * negndotl * negndotl;
}

float normalDistribution(float ndotm, float alphaSqr) {

    float ndotmSq = ndotm * ndotm;

    return exp((ndotmSq - 1) / (alphaSqr * ndotmSq)) / (PI * alphaSqr * ndotmSq * ndotmSq);
}

float geometricShadowing(float ndotv, float alpha) {
    float k = alpha * 0.5f;
    return ndotv / (ndotv * (1 - k) + k);
}

vec3 specularBrdf(vec3 l, vec3 v, vec3 n, float roughness, vec3 f0) {
    vec3 h = normalize((l + v) * 0.5f);
    float alpha = roughness * roughness;

    vec3 nom = fresnel(f0, dot(h, l)) * geometricShadowing(dot(n, v), alpha) * normalDistribution(dot(n, h), alpha * alpha);
    float denom = 4 * dot(n, l) * dot(n, v);

    return nom / denom;
}

vec3 importanceSampleBeckmann(vec2 xi, vec3 N, float roughness)
{
    float alpha = roughness * roughness;

    float tanThetaSquared =
        -alpha * alpha * log(max(1.0 - xi.x, 1e-7));

    float cosTheta = inversesqrt(1.0 + tanThetaSquared);
    float sinTheta = sqrt(max(1.0 - cosTheta * cosTheta, 0.0));
    float phi = 2.0 * PI * xi.y;

    vec3 localH = vec3(
        sinTheta * cos(phi),
        sinTheta * sin(phi),
        cosTheta
    );

    vec3 helper = abs(N.z) < 0.999
        ? vec3(0.0, 0.0, 1.0)
        : vec3(1.0, 0.0, 0.0);

    vec3 T = normalize(cross(helper, N));
    vec3 B = cross(N, T);

    return normalize(T * localH.x + B * localH.y + N * localH.z);
}

#endif