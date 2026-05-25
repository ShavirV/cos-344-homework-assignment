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
uniform int       hasTexDay;
uniform int       hasTexNight;
uniform float     texBlend;      // 0.0 = full night, 1.0 = full day
uniform sampler2D skyTexDay;
uniform sampler2D skyTexNight;

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
    if (hasTexDay == 1 || hasTexNight == 1) {
        vec3  ldir = normalize(localPos);
        float lon  = atan(ldir.z, ldir.x) / (2.0 * 3.14159265) + 0.5;
        float lat  = asin(clamp(ldir.y, -1.0, 1.0)) / 3.14159265 + 0.5;
        // Tile the panorama horizontally so each copy of the BMP covers less
        // sky and pixels appear sharper ("zoomed out"). GL_REPEAT on S in
        // Skybox.cpp lets longitude tile cleanly. Latitude is GL_CLAMP_TO_EDGE
        // so it must stay in [0, 1] — don't scale lat.
        const float TILE_S = 3.0;
        vec2  suv  = vec2(lon * TILE_S, lat);

        vec3 dayCol   = skyColor;   // fallback to gradient if texture missing
        vec3 nightCol = skyColor;

        if (hasTexDay   == 1) dayCol   = texture(skyTexDay,   suv).rgb;
        if (hasTexNight == 1) nightCol = texture(skyTexNight, suv).rgb;

        // Smooth transition zone — blend over a ~20% intensity window
        // so the crossfade happens around sunrise/sunset, not abruptly.
        float blend = smoothstep(0.1, 0.3, texBlend);
        vec3 texColor = mix(nightCol, dayCol, blend);

        skyColor = mix(skyColor, texColor, 0.85);
    }

    // ── Sun disc ──────────────────────────────────────────────────────────────
    // Sun is detected in ROTATED space (worldDir) against sunLocalDir rotated
    // by the same matrix — but since getSun() rotates sunLocalDir, and here
    // we compare worldDir (also rotated), we actually want to compare against
    // the LOCAL dir of the sun in the sphere's own frame.
    // localPos is pre-rotation, sunLocalDir is also pre-rotation → compare those.
    float sunF  = discFactor(worldDir, sunLocalDir,  sunDiscCos,  0.08);
    float moonF = discFactor(worldDir, moonLocalDir, moonDiscCos, 0.05);

    // Sun glow: brighten the sky around the sun
    float sunGlow  = clamp(dot(normalize(worldDir), normalize(sunLocalDir)),
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
    float moonGlow = clamp(dot(normalize(worldDir), normalize(moonLocalDir)),   
                           0.0, 1.0);
    moonGlow = pow(moonGlow, 12.0) * 0.08;
    color += moonGlow * moonColor;

    // Tone-map slightly to avoid overexposure
    color = color / (color + vec3(0.6));
    color = pow(color, vec3(1.0 / 2.2));  // gamma correct

    FragColor = vec4(color, 1.0);
}