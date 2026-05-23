#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat3 uNormalMat;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos     = worldPos.xyz;
    vNormal       = normalize(uNormalMat * aNormal);
    vColor        = aColor;
    gl_Position   = uMVP * vec4(aPos, 1.0);
}