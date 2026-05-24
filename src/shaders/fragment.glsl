#version 330 core

// =============================================================================
// fragment.glsl
// Blinn-Phong shading with procedural-noise "textures" for turf and kerb
// materials, plus a hemisphere ambient term.
// =============================================================================

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform vec3 uViewPos;

out vec4 FragColor;

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float vnoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
    float v = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 3; ++i) {
        v += amp * vnoise(p);
        p *= 2.07;
        amp *= 0.5;
    }
    return v;
}

vec3 shadeTurf(vec3 baseGreen, vec3 worldPos) {
    vec2 uv = worldPos.xz;
    float n1 = vnoise(uv * 8.0);
    float n2 = fbm(uv * 0.6);
    vec3 coolGreen = baseGreen * vec3(0.85, 1.00, 0.70);
    vec3 warmGreen = baseGreen * vec3(1.15, 1.00, 0.55);
    vec3 tinted    = mix(coolGreen, warmGreen, n1);
    float wear = smoothstep(0.45, 0.65, n2);
    tinted *= mix(1.0, 0.70, wear);
    float speckle = vnoise(uv * 35.0) * 0.10 - 0.05;
    tinted += vec3(speckle * 0.5, speckle, speckle * 0.3);
    return tinted;
}

vec3 shadeKerb(vec3 baseGrey, vec3 worldPos) {
    vec2 uv = worldPos.xz;
    float aggregate = vnoise(uv * 25.0) * 0.20 - 0.10;
    float weather   = fbm(uv * 0.8);
    vec3 fresh   = baseGrey * 1.05;
    vec3 stained = baseGrey * vec3(0.85, 0.88, 0.92);
    vec3 tinted  = mix(stained, fresh, smoothstep(0.35, 0.65, weather));
    tinted += vec3(aggregate);
    return tinted;
}

bool looksLikeTurf(vec3 c) {
    return c.g > c.r + 0.15 && c.g > c.b + 0.15 && c.g > 0.30;
}
bool looksLikeKerb(vec3 c) {
    float maxc = max(c.r, max(c.g, c.b));
    float minc = min(c.r, min(c.g, c.b));
    return (maxc - minc) < 0.10 && maxc > 0.50;
}

void main() {
    vec3 base;
    if (looksLikeTurf(vColor))      base = shadeTurf(vColor, vWorldPos);
    else if (looksLikeKerb(vColor)) base = shadeKerb(vColor, vWorldPos);
    else                            base = vColor;

    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 H = normalize(L + V);

    vec3 skyColor     = vec3(0.55, 0.70, 0.95);
    vec3 groundBounce = uAmbient * 1.20;
    float hemi = 0.5 + 0.5 * N.y;
    vec3 ambient = mix(groundBounce, skyColor, hemi) * 0.75;

    float diff = max(dot(N, L), 0.0);

    float specStrength = 0.10;
    float specPower    = 24.0;
    if (looksLikeTurf(vColor)) { specStrength = 0.02; specPower = 8.0;  }
    if (looksLikeKerb(vColor)) { specStrength = 0.18; specPower = 32.0; }
    float spec = pow(max(dot(N, H), 0.0), specPower) * specStrength;

    vec3 lit = base * (ambient + diff * uLightColor) + spec * uLightColor;
    lit = lit / (1.0 + 0.4 * lit);

    FragColor = vec4(lit, 1.0);
}
#version 330 core
out vec4 FragColor;
uniform vec4 colour;

void main()
{
    FragColor = colour;
}
