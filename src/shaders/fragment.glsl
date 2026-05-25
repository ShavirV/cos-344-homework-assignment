#version 330 core

// =============================================================================
// fragment.glsl
// Blinn-Phong shading with procedural-noise "textures" for turf and kerb
// materials, plus a hemisphere ambient term.
//
// Shadow mapping (NEW):
//   - vLightSpacePos comes from the vertex shader (= uLightSpaceMatrix * worldPos)
//   - uShadowMap is the depth texture rendered from the sun's POV in the
//     pre-pass. We sample it with a 3x3 PCF kernel for soft edges.
//   - Only diffuse + specular contributions are shadowed. Ambient stays
//     unaffected so deeply-shadowed areas remain readable.
// =============================================================================

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;
in vec4 vLightSpacePos;          // NEW

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform vec3 uViewPos;

// NEW — the depth texture produced by the shadow-map pre-pass. When the
// caller hasn't bound a texture, sampling returns 0 on most drivers and
// the shadow term silently does nothing; the explicit `uHasShadowMap`
// flag below makes the bypass deterministic.
uniform sampler2D uShadowMap;
uniform int       uHasShadowMap; // 0 = no shadow, 1 = sample the map

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

// =============================================================================
// shadowFactor — returns 0 (fully lit) to 1 (fully in shadow)
//
// Implementation:
//   1. Perspective divide (lightSpacePos.xyz / .w) → light-clip [-1,1].
//      Orthographic light matrices keep .w=1, but doing the divide is safe
//      either way.
//   2. Remap to [0,1] for texture lookup.
//   3. Bail out when the fragment is outside the light's frustum (z > 1 or
//      any coord outside [0,1]) — treat as fully lit so the world OUTSIDE
//      the shadow map area is normally lit.
//   4. Bias: subtract a small constant from the fragment's depth to avoid
//      "shadow acne" — surfaces shadowing themselves due to depth precision.
//      The bias scales with the surface angle to the light: surfaces at
//      grazing angles need more bias than ones facing the sun head-on.
//   5. 3×3 PCF (percentage-closer filtering): sample 9 neighbouring texels
//      and average. Soft shadow edges instead of pixelated bitmaps.
// =============================================================================
float shadowFactor(vec4 lightSpacePos, vec3 N, vec3 L)
{
    if (uHasShadowMap == 0) return 0.0;

    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Outside the shadow map frustum → no shadow.
    if (projCoords.z > 1.0) return 0.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;

    // Bias: 0.0005 minimum, scales with how oblique the light is to the
    // surface (up to 0.005 at grazing angles). Tweak these two numbers if
    // you see acne (raise) or "Peter Panning" — shadows floating off the
    // ground (lower).
    float bias = max(0.005 * (1.0 - dot(N, L)), 0.0005);
    float currentDepth = projCoords.z - bias;

    // 3×3 PCF using textureSize for the kernel offset.
    float shadow   = 0.0;
    vec2  texel    = 1.0 / vec2(textureSize(uShadowMap, 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closestDepth =
                texture(uShadowMap, projCoords.xy + vec2(x, y) * texel).r;
            shadow += (currentDepth > closestDepth) ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
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

    // NEW — sample the shadow map and modulate the direct-light contributions.
    // Ambient stays unaffected so shadowed areas don't go pitch-black.
    float shadow = shadowFactor(vLightSpacePos, N, L);
    float litFactor = 1.0 - shadow;

    vec3 lit = base * (ambient + diff * uLightColor * litFactor)
             + spec * uLightColor * litFactor;
    lit = lit / (1.0 + 0.4 * lit);

    FragColor = vec4(lit, 1.0);
}