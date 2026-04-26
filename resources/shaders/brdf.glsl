#ifndef BRDF_GLSL__
#define BRDF_GLSL__

#define PI 3.14159265

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

vec3 specular_brdf(vec3 l, vec3 v, vec3 n, float roughness, vec3 f0) {
    vec3 h = (l + v) * 0.5f;
    float alpha = roughness * roughness;

    vec3 nom = fresnel(f0, dot(h, l)) * geometricShadowing(dot(n, v), alpha) * normalDistribution(dot(n, h), alpha * alpha);
    float denom = 4 * dot(n, l) * dot(n, v);

    return nom / denom;
}

#endif