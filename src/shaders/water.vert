#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 ViewDir;
out float WaveHeight;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;
uniform float time;

// waves[i] = vec4(dirX, dirZ, steepness, wavelength)
uniform vec4 waves[4];

const float PI      = 3.14159265;
const float GRAVITY = 9.8;

vec3 GerstnerWave(vec4 wave, vec3 p, inout vec3 tangent, inout vec3 binormal)
{
    float steepness  = wave.z;
    float wavelength = wave.w;
    float k          = 2.0 * PI / wavelength;
    float c          = sqrt(GRAVITY / k);
    vec2  d          = normalize(wave.xy);
    float f          = k * (dot(d, p.xz) - c * time);
    float a          = steepness / k;

    tangent  += vec3(-d.x*d.x*steepness*sin(f),
                      d.x*steepness*cos(f),
                     -d.x*d.y*steepness*sin(f));
    binormal += vec3(-d.x*d.y*steepness*sin(f),
                      d.y*steepness*cos(f),
                     -d.y*d.y*steepness*sin(f));

    return vec3(d.x*a*cos(f), a*sin(f), d.y*a*cos(f));
}

void main()
{
    vec3 p        = aPos;
    vec3 tangent  = vec3(1.0, 0.0, 0.0);
    vec3 binormal = vec3(0.0, 0.0, 1.0);

    for (int i = 0; i < 4; i++)
        p += GerstnerWave(waves[i], aPos, tangent, binormal);

    WaveHeight = p.y;
    vec3 normal = normalize(cross(binormal, tangent));

    vec4 worldPos = model * vec4(p, 1.0);
    FragPos       = worldPos.xyz;
    Normal        = mat3(transpose(inverse(model))) * normal;
    TexCoord      = aTexCoord;
    ViewDir       = normalize(viewPos - worldPos.xyz);

    gl_Position   = projection * view * worldPos;
}