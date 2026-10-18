#version 450

/**
 * A full-screen pass that copies its source, a texel per pixel. It reads by position with
 * texelFetch, not through a sampler, so no filtering applies. It writes exactly what it reads.
 **/

layout(set = 1, binding = 0) uniform sampler2D source;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 colour;

void main() {
    colour = texelFetch(source, ivec2(gl_FragCoord.xy), 0);
}
