#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;

out vec3 localPos;    // un-rotated position on unit sphere
out vec3 worldDir;    // rotated direction (for lighting)
out vec2 uv;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 rotation;   // day/night rotation around X axis
uniform float radius;

void main()
{
    // aPos is on a unit sphere — scale to skybox radius
    vec3 scaled = aPos * radius;

    // localPos used in fragment shader to determine latitude
    // for gradient and disc detection (before rotation)
    localPos = aPos;

    // worldDir is the rotated direction — used to find where sun/moon are
    worldDir = vec3(rotation * vec4(aPos, 0.0));

    uv = aUV;

    // View with no translation (strip last column) so sky stays centred
    gl_Position = projection * view * vec4(scaled, 1.0);

    // Push to far plane — ensure skybox is always behind scene geometry
    gl_Position = gl_Position.xyww;
}