#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;

uniform vec3  uLightDir;
uniform vec3  uLightColor;
uniform vec3  uAmbient;
uniform vec3  uViewPos;

uniform float uEmissive;
uniform vec3  uEmissiveColor;

uniform vec3  uLavaLightPos;
uniform vec3  uLavaLightColor;
uniform float uLavaLightIntensity;

out vec4 FragColor;

float hash3(vec3 p) {
    p = fract(p * vec3(127.1, 311.7, 74.7));
    p += dot(p, p.yxz + 19.19);
    return fract((p.x + p.y) * p.z);
}

float noise3(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(mix(hash3(i),              hash3(i + vec3(1,0,0)), f.x),
            mix(hash3(i + vec3(0,1,0)), hash3(i + vec3(1,1,0)), f.x), f.y),
        mix(mix(hash3(i + vec3(0,0,1)), hash3(i + vec3(1,0,1)), f.x),
            mix(hash3(i + vec3(0,1,1)), hash3(i + vec3(1,1,1)), f.x), f.y),
        f.z);
}

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 H = normalize(L + V);

    // Procedural micro-variation (no texture needed)
    float nv  = noise3(vWorldPos * 2.5) * 0.06 - 0.03;
    vec3  base = clamp(vColor + nv, 0.0, 1.0);

    // Blinn-Phong diffuse + specular
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 28.0) * 0.18;
    vec3  lit  = base * (uAmbient + diff * uLightColor) + spec * uLightColor;

    // Secondary lava point light
    if (uLavaLightIntensity > 0.001) {
        vec3  lv    = uLavaLightPos - vWorldPos;
        float ld    = length(lv);
        lv         /= ld;
        float la    = max(dot(N, lv), 0.0);
        float atten = uLavaLightIntensity / (1.0 + 0.04 * ld * ld);
        lit        += base * la * uLavaLightColor * atten;
    }

    // Rim light
    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.5) * 0.12;
    lit += rim * uLightColor;

    // Emissive override (lava surfaces self-illuminate)
    if (uEmissive > 0.5) {
        float shimmer = noise3(vWorldPos * 3.0) * 0.3 + 0.7;
        lit = base * uEmissiveColor * shimmer * 3.5;
    }

    // Reinhard tone-map + gamma correction
    lit = lit / (lit + vec3(1.0));
    lit = pow(clamp(lit, 0.0, 1.0), vec3(1.0 / 2.2));

    FragColor = vec4(lit, 1.0);
}