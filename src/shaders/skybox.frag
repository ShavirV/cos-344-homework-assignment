#version 330 core

in vec3 localPos;   // position on unit sphere BEFORE rotation
in vec3 worldDir;   // position on unit sphere AFTER rotation
in vec2 uv;

out vec4 FragColor;

uniform vec3  skyTopColor;
uniform vec3  skyHorizColor;
uniform float radius;

// Sun / moon disc
uniform vec3  sunLocalDir;    // direction of sun in LOCAL (pre-rotation) space
uniform vec3  moonLocalDir;
uniform vec3  sunColor;
uniform vec3  moonColor;
uniform float sunDiscCos;     // cos(half angular size) — dot threshold for disc
uniform float moonDiscCos;

// Optional sky panorama texture
uniform int       hasTexture;
uniform sampler2D skyTex;

// ── Helpers ──────────────────────────────────────────────────────────────────

// Smooth disc with a soft glow corona
float discFactor(vec3 dir, vec3 discDir, float edgeCos, float glowWidth)
{
    float d = dot(normalize(dir), normalize(discDir));

    // Hard disc
    float disc = smoothstep(edgeCos - 0.003, edgeCos + 0.003, d);

    // Soft glow corona around the disc
    float glowCos = edgeCos - glowWidth;
    float glow    = smoothstep(glowCos, edgeCos, d) * 0.35;

    return clamp(disc + glow, 0.0, 1.0);
}

void main()
{
    vec3 dir = normalize(worldDir);   // rotated direction

    // ── Sky colour ────────────────────────────────────────────────────────────
    // Use worldDir.y (height after rotation) for gradient so it tracks the
    // sun: sky is brighter/bluer where the sun is, darker on the night side.
    float horizBlend = clamp(dir.y * 2.0 + 0.5, 0.0, 1.0);  // 0=below, 1=above
    vec3  skyColor   = mix(skyHorizColor, skyTopColor, horizBlend);

    // ── Texture (panorama) ────────────────────────────────────────────────────
    if (hasTexture == 1) {
        // localPos is the un-rotated direction — sample panorama in rest frame
        vec3 ldir = normalize(localPos);
        // Spherical UV: u = longitude [0,1], v = latitude [0,1]
        float lon = atan(ldir.z, ldir.x) / (2.0 * 3.14159265) + 0.5;
        float lat = asin(clamp(ldir.y, -1.0, 1.0)) / 3.14159265 + 0.5;
        vec4 texColor = texture(skyTex, vec2(lon, lat));
        // Blend texture with procedural gradient (texture on top)
        skyColor = mix(skyColor, texColor.rgb, 0.85);
    }

    // ── Sun disc ──────────────────────────────────────────────────────────────
    // Sun is detected in ROTATED space (worldDir) against sunLocalDir rotated
    // by the same matrix — but since getSun() rotates sunLocalDir, and here
    // we compare worldDir (also rotated), we actually want to compare against
    // the LOCAL dir of the sun in the sphere's own frame.
    // localPos is pre-rotation, sunLocalDir is also pre-rotation → compare those.
    float sunF  = discFactor(localPos, sunLocalDir,  sunDiscCos,  0.08);
    float moonF = discFactor(localPos, moonLocalDir, moonDiscCos, 0.05);

    // Sun glow: brighten the sky around the sun
    float sunGlow  = clamp(dot(normalize(localPos), normalize(sunLocalDir)),
                           0.0, 1.0);
    sunGlow = pow(sunGlow, 6.0) * 0.4;

    // Horizon haze: warm orange band at the horizon (worldDir.y near 0)
    float horizon = exp(-abs(dir.y) * 4.5) * 0.35;
    vec3  hazeCol = vec3(0.90, 0.55, 0.25);

    // ── Combine ───────────────────────────────────────────────────────────────
    vec3 color = skyColor;
    color += sunGlow  * sunColor;          // sun glow halo
    color += horizon  * hazeCol;           // horizon haze
    color  = mix(color, sunColor,  sunF);  // hard sun disc
    color  = mix(color, moonColor, moonF); // hard moon disc

    // Moon has a subtle glow too
    float moonGlow = clamp(dot(normalize(localPos), normalize(moonLocalDir)),
                           0.0, 1.0);
    moonGlow = pow(moonGlow, 12.0) * 0.08;
    color += moonGlow * moonColor;

    // Tone-map slightly to avoid overexposure
    color = color / (color + vec3(0.6));
    color = pow(color, vec3(1.0 / 2.2));  // gamma correct

    FragColor = vec4(color, 1.0);
}
