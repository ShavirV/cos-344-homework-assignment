#version 330 core
// Passthrough vertex shader - all transforms done on CPU
// Attribute 0: position (x, y) - already in NDC
// Attribute 1: colour (r, g, b)

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColour;

out vec3 vColour;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vColour = aColour;
}
