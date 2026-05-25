#version 330 core

// =============================================================================
// shadow.vert — depth-only shadow map vertex shader
//
// Runs during the shadow map pre-pass. Transforms each vertex into the
// light's clip space; OpenGL writes the resulting gl_Position.z (after
// perspective divide and viewport transform) into the depth attachment of
// the shadow framebuffer. That depth value is what the main fragment
// shader later compares against to test for shadow.
//
// IMPORTANT: this shader binds attribute location 0 (position) only.
// Mesh.cpp's setup() leaves slot 1 (normal) and slot 2 (color) bound but
// unused; the GPU ignores them when the shader doesn't reference them.
// =============================================================================

layout(location = 0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 uLightSpaceMatrix;   // = lightProj * lightView

void main()
{
    gl_Position = uLightSpaceMatrix * uModel * vec4(aPos, 1.0);
}