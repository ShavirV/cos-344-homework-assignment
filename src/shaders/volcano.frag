#version 330 core

// Volcano fragment shader.
// Rock surfaces use Phong with the vertex color as base color.
// Lava (detected from the vertex color: red-dominant + warm) is rendered as
// emissive so it visibly glows even at night without a real light source.

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec3 vColor;

uniform vec3 viewPos;       // camera position in world space
uniform vec3 lightDir;      // unit vector pointing FROM surface TOWARD the light
                            // (same convention as fragment.glsl's uLightDir)
uniform vec3 lightColor;    // e.g. vec3(1.0, 0.95, 0.85)
uniform vec3 ambientColor;  // e.g. vec3(0.15, 0.16, 0.20)
uniform float time;         // seconds, used for lava flicker

// NEW — volcano point light. The crater glows on the volcano's own rocky
// flanks too (and far more strongly there than anywhere else in the scene
// since it's just below the rim).
uniform vec3 uVolcanoLightPos;
uniform vec3 uVolcanoLightColor;

out vec4 FragColor;

// Cheap pseudo-random
float hash(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightDir);          // already points toward the light
    vec3 V = normalize(viewPos - vWorldPos);
    vec3 H = normalize(L + V);

    // Detect "lava-ness" from vertex color: warm, red-dominant tones.
    float warmth = clamp(vColor.r - 0.5 * (vColor.g + vColor.b), 0.0, 1.0);
    float lavaMix = smoothstep(0.05, 0.35, warmth);

    // --- Rock shading (Phong) ---
    vec3 rockBase = vColor;
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 24.0) * 0.25;
    vec3 rockShade = ambientColor * rockBase
                   + diff * lightColor * rockBase
                   + spec * lightColor;

    // ── Volcano point light on the rock (NEW) ─────────────────────────────
    // The crater illuminates the rocky flanks below it. Falloff is the same
    // shape as in fragment.glsl so the volcano's edge glow matches the
    // glow it casts on holes / dinosaurs.
    vec3 toCrater  = uVolcanoLightPos - vWorldPos;
    float craterD  = length(toCrater);
    vec3 craterL   = (craterD > 1e-4) ? (toCrater / craterD) : vec3(0.0, 1.0, 0.0);
    float craterAtt = 1.0 / (1.0 + 0.014 * craterD + 0.0007 * craterD * craterD);
    float craterDiff = max(dot(N, craterL), 0.0);
    rockShade += rockBase * uVolcanoLightColor * craterDiff * craterAtt;

    // --- Lava: emissive with flicker, plus a hot glow ---
    float flicker = 0.85 + 0.15 * sin(time * 6.0 + hash(floor(vWorldPos * 4.0)) * 6.28);
    vec3 lavaGlow = vColor * flicker * 1.6;
    // Fresnel rim so lava edges shimmer
    float fres = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    lavaGlow += vec3(1.0, 0.5, 0.15) * fres * 0.4 * lavaMix;

    vec3 finalColor = mix(rockShade, lavaGlow, lavaMix);
    FragColor = vec4(finalColor, 1.0);
}