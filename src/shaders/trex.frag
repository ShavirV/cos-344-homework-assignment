#version 330 core

// T-rex fragment shader.
// Blinn-Phong with a procedural scale pattern overlay on greenish skin,
// flatter highlights on the cream belly, and a brighter spec on
// teeth/claws (light vs. dark colors).
//
// IMPORTANT: lightDir uses the SAME convention as fragment.glsl in your
// project - it points FROM the surface TOWARD the light. Pass the same
// value you give the hole shader's uLightDir.

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec3 vColor;

uniform vec3 viewPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;

out vec4 FragColor;

float hash(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453);
}

float vnoise3(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float n000 = hash(i);
    float n100 = hash(i + vec3(1,0,0));
    float n010 = hash(i + vec3(0,1,0));
    float n110 = hash(i + vec3(1,1,0));
    float n001 = hash(i + vec3(0,0,1));
    float n101 = hash(i + vec3(1,0,1));
    float n011 = hash(i + vec3(0,1,1));
    float n111 = hash(i + vec3(1,1,1));
    return mix(
        mix(mix(n000, n100, f.x), mix(n010, n110, f.x), f.y),
        mix(mix(n001, n101, f.x), mix(n011, n111, f.x), f.y),
        f.z
    );
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightDir);          // from surface toward light
    vec3 V = normalize(viewPos - vWorldPos);
    vec3 H = normalize(L + V);

    // Classify vertex by color luminance + hue to pick material tweaks
    float lum = dot(vColor, vec3(0.299, 0.587, 0.114));
    bool isToothOrClaw = (lum > 0.78) || (lum < 0.15);
    bool isMouth       = vColor.r > vColor.g + 0.20 && vColor.r > vColor.b + 0.20;
    bool isGreenSkin   = (vColor.g > vColor.r + 0.02) && (vColor.g > vColor.b + 0.05);

    // Procedural scale pattern: cellular pseudo-bumps on green skin only.
    // Tiles a noisy bump pattern over the body to break up the flat color.
    vec3 base = vColor;
    if (isGreenSkin) {
        float scale = vnoise3(vWorldPos * 6.0);
        float ridge = vnoise3(vWorldPos * 18.0);
        // darker creases between scales
        float crease = smoothstep(0.45, 0.65, scale) * 0.25;
        base *= (1.0 - crease);
        // tiny lighter scale highlights
        float hi = smoothstep(0.70, 0.95, ridge) * 0.18;
        base += vec3(hi * 0.7, hi, hi * 0.6);
    }

    // Hemisphere ambient: brighter from sky, darker from ground
    vec3 skyColor     = vec3(0.55, 0.70, 0.95);
    vec3 groundBounce = ambientColor * 1.10;
    float hemi = 0.5 + 0.5 * N.y;
    vec3 ambient = mix(groundBounce, skyColor, hemi) * 0.65;

    float diff = max(dot(N, L), 0.0);

    // Per-material spec
    float specStrength = 0.10;
    float specPower    = 18.0;
    if (isToothOrClaw) { specStrength = 0.45; specPower = 64.0; }
    if (isMouth)       { specStrength = 0.30; specPower = 32.0; }
    float spec = pow(max(dot(N, H), 0.0), specPower) * specStrength;

    vec3 lit = base * (ambient + diff * lightColor) + spec * lightColor;
    // Soft tonemap
    lit = lit / (1.0 + 0.4 * lit);

    FragColor = vec4(lit, 1.0);
}
