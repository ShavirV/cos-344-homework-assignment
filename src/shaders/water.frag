#version 330 core

in vec3  FragPos;
in vec3  Normal;
in vec2  TexCoord;
in vec3  ViewDir;
in float WaveHeight;

out vec4 FragColor;

uniform sampler2D normalMap0;
uniform sampler2D normalMap1;
uniform sampler2D foamMap;

uniform vec3  lightDir;
uniform vec3  lightColor;
uniform vec3  waterColorShallow;
uniform vec3  waterColorDeep;
uniform float time;

uniform float normalTile0;
uniform float normalTile1;
uniform float scrollSpeed0;
uniform float scrollSpeed1;
uniform float foamTile;
uniform float foamThreshold;

uniform float fresnelBias;
uniform float fresnelScale;
uniform float fresnelPower;
uniform float specularStrength;
uniform float shininess;

// Reoriented Normal Mapping blend
vec3 blendNormals(vec3 n1, vec3 n2) {
    n1 += vec3(0.0, 0.0, 1.0);
    n2 *= vec3(-1.0, -1.0, 1.0);
    return normalize(n1 * dot(n1, n2) - n2 * n1.z);
}

float fresnel(vec3 V, vec3 N) {
    float f = fresnelBias + fresnelScale
              * pow(1.0 - max(dot(V, N), 0.0), fresnelPower);
    return clamp(f, 0.0, 1.0);
}

void main()
{
    // Scrolling UVs — river flows along +Z (TexCoord.y)
    vec2 uv0 = TexCoord * normalTile0;
    uv0.y   -= time * scrollSpeed0;

    vec2 uv1 = TexCoord * normalTile1;
    uv1.y   -= time * scrollSpeed1;
    uv1.x   += time * scrollSpeed1 * 0.3;

    // Sample and blend normal maps
    vec3 nm0 = texture(normalMap0, uv0).rgb * 2.0 - 1.0;
    vec3 nm1 = texture(normalMap1, uv1).rgb * 2.0 - 1.0;
    vec3 mapN = normalize(blendNormals(nm0, nm1));
    vec3 N    = normalize(Normal + mapN * 0.6);

    // Lighting
    vec3 L = normalize(lightDir);
    vec3 V = normalize(ViewDir);
    vec3 H = normalize(L + V);

    float diff    = max(dot(N, L), 0.0);
    float spec    = pow(max(dot(N, H), 0.0), shininess);
    vec3  ambient = vec3(0.15, 0.20, 0.25);
    vec3  diffuse = diff * lightColor;
    vec3  specular= specularStrength * spec * lightColor;

    // Water colour (Fresnel-driven depth blend)
    float f         = fresnel(V, N);
    vec3  waterColor= mix(waterColorDeep, waterColorShallow, 1.0 - f);

    // Foam / whitecaps
    vec2  fuv     = TexCoord * foamTile;
    fuv.y        -= time * scrollSpeed0 * 0.8;
    float foamTex = texture(foamMap, fuv).r;
    float foamMask= smoothstep(foamThreshold - 0.05, foamThreshold + 0.05, WaveHeight);
    float foam    = foamMask * foamTex;

    // Combine
    vec3 color = waterColor * (ambient + diffuse) + specular;
    color      = mix(color, vec3(1.0), foam * 0.8);
    color      = mix(color, vec3(0.2, 0.45, 0.5), 0.08);

    float alpha = mix(0.75, 0.96, f);
    FragColor   = vec4(color, alpha);
}