#version 450

/**
 * A full-screen pass that copies its source, a texel per pixel, read by position rather than
 * through a sampler so that no filtering can touch it. What it writes is what was read.
 **/

layout(set = 1, binding = 0) uniform sampler2D source;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 colour;

void main() {
    colour = texelFetch(source, ivec2(gl_FragCoord.xy), 0);
}
