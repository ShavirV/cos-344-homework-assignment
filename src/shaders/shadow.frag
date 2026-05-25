#version 330 core

// =============================================================================
// shadow.frag — depth-only shadow map fragment shader
//
// Empty body: GL writes gl_FragCoord.z into the bound depth attachment
// automatically. We don't need any colour output. (A few drivers reject
// a no-shader pipeline, hence this stub.)
// =============================================================================

void main() {}