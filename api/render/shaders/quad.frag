#version 450

/**
 * The fragment half of the one batched quad primitive - ADR-0005.
 *
 * There is one sampler and it is always bound, because an untextured quad is drawn against a
 * 1x1 white texture rather than through a second pipeline. A single channel texture - a glyph
 * atlas - is given a view that swizzles its one channel into alpha, so the sample reaches the
 * same place whatever was bound.
 *
 * The one branch is text, per ADR-0036: a glyph atlas holds signed distances rather than
 * coverage, and a distance has to be thresholded to become an alpha. It is uniform across a
 * draw, since a batch is text or is not.
 **/

layout(location = 0) in vec2 fragmentUv;
layout(location = 1) in vec4 fragmentColour;

layout(location = 0) out vec4 outColour;

// set 1 is the per material frequency, per docs/RenderingPipeline.md
layout(set = 1, binding = 0) uniform sampler2D albedo;

layout(push_constant) uniform Push {
    mat4 projection;
    uint text;
} push;

void main() {
    vec4 texel = texture(albedo, fragmentUv);

    if (push.text != 0u) {
        // the glyph's edge is where the distance crosses the midpoint the field was built
        // around. Blending across one pixel of it rather than cutting at it is what makes
        // the edge smooth, and taking that pixel's width from how fast the distance changes
        // on screen is what keeps it one pixel wide however far the glyph has been scaled
        float distance = texel.a;
        float width = fwidth(distance);
        float coverage = smoothstep(0.5 - width, 0.5 + width, distance);
        outColour = vec4(fragmentColour.rgb, fragmentColour.a * coverage);
    } else {
        outColour = fragmentColour * texel;
    }
}
