#version 330 core

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
    vec3 L = normalize(lightDir);
    vec3 V = normalize(viewPos - vWorldPos);
    vec3 H = normalize(L + V);

    // Classify by luminance: very dark = nail/horn, very bright = eye
    float lum = dot(vColor, vec3(0.299, 0.587, 0.114));
    bool isHardSurface = (lum > 0.78) || (lum < 0.15);

    // Procedural skin: cellular/wrinkle break-up on regular skin only.
    vec3 base = vColor;
    if (!isHardSurface) {
        float coarse = vnoise3(vWorldPos * 3.5);
        float fine   = vnoise3(vWorldPos * 12.0);
        // darker creases between skin scales
        float crease = smoothstep(0.48, 0.68, coarse) * 0.20;
        base *= (1.0 - crease);
        // subtle lighter scales
        float hi = smoothstep(0.70, 0.95, fine) * 0.15;
        base += vec3(hi * 0.8, hi * 0.7, hi * 0.5);
    }

    // Hemisphere ambient
    vec3 skyColor     = vec3(0.55, 0.70, 0.95);
    vec3 groundBounce = ambientColor * 1.10;
    float hemi = 0.5 + 0.5 * N.y;
    vec3 ambient = mix(groundBounce, skyColor, hemi) * 0.65;

    float diff = max(dot(N, L), 0.0);

    // Per-material spec
    float specStrength = 0.08;
    float specPower    = 16.0;
    if (isHardSurface) { specStrength = 0.40; specPower = 56.0; }
    float spec = pow(max(dot(N, H), 0.0), specPower) * specStrength;

    vec3 lit = base * (ambient + diff * lightColor) + spec * lightColor;
    lit = lit / (1.0 + 0.4 * lit);   // soft tonemap

    FragColor = vec4(lit, 1.0);
}
