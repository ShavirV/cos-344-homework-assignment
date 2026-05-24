#version 330 core

// =============================================================================
// vertex.glsl
// Standard pass-through vertex shader used by all hole geometry and the
// camera drone.
//
// view and projection are uploaded once per frame by dev.cpp. uModel is
// uploaded per draw call by each hole/mesh class. aColor in slot 2 carries
// the surface base colour — either per-vertex (Hole13/14/15/Drone) or as a
// constant attribute set via glVertexAttrib3f (Mesh, for Hole16).
// =============================================================================

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 uModel;
uniform mat3 uNormalMat;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;

void main()
{
    vec4 wp   = uModel * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    vNormal   = normalize(uNormalMat * aNormal);
    vColor    = aColor;
    gl_Position = projection * view * vec4(wp.xyz, 1.0);
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}