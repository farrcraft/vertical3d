#version 450

/**
 * The fragment half of the one batched quad primitive - ADR-0005.
 *
 * There is one sampler and it is always bound, because an untextured quad is drawn against a
 * 1x1 white texture rather than through a second pipeline. A single channel texture - a glyph
 * atlas - is given a view that swizzles its one channel into alpha, so text needs no branch
 * here either.
 **/

layout(location = 0) in vec2 fragmentUv;
layout(location = 1) in vec4 fragmentColour;

layout(location = 0) out vec4 outColour;

// set 1 is the per material frequency, per docs/RenderingPipeline.md
layout(set = 1, binding = 0) uniform sampler2D albedo;

void main() {
    outColour = fragmentColour * texture(albedo, fragmentUv);
}
