#version 330 core

// Brachiosaurus vertex shader -- matches Brachio.hpp layout.

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out vec3 vColor;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    mat3 normalMat = transpose(inverse(mat3(model)));
    vNormal = normalize(normalMat * aNormal);
    vUV = aUV;
    vColor = aColor;
    gl_Position = projection * view * worldPos;
}
