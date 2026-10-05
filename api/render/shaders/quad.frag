#version 450

/**
 * The fragment stage of the batched quad primitive.
 *
 * There is one sampler and it is always bound, because an untextured quad is drawn against a
 * 1x1 white texture rather than through a second pipeline. A single channel texture - a glyph
 * atlas - is given a view that swizzles its one channel into alpha, so the sample reaches the
 * same place whatever was bound.
 *
 * The only branch is text. A glyph atlas holds signed distances rather than coverage, and a
 * distance has to be thresholded to become an alpha. The branch is uniform across a draw,
 * because a batch is either all text or none.
 **/

layout(location = 0) in vec2 fragmentUv;
layout(location = 1) in vec4 fragmentColour;

layout(location = 0) out vec4 outColour;

// set 1 holds per material data
layout(set = 1, binding = 0) uniform sampler2D albedo;

layout(push_constant) uniform Push {
    mat4 projection;
    uint text;
} push;

void main() {
    vec4 texel = texture(albedo, fragmentUv);

    if (push.text != 0u) {
        // the glyph's edge is where the distance crosses the midpoint the field was built
        // around. Blending across one pixel of it, rather than cutting at it, makes the edge
        // smooth. The pixel's width comes from how fast the distance changes on screen, so the
        // blend stays one pixel wide however far the glyph has been scaled
        float distance = texel.a;
        float width = fwidth(distance);
        float coverage = smoothstep(0.5 - width, 0.5 + width, distance);
        outColour = vec4(fragmentColour.rgb, fragmentColour.a * coverage);
    } else {
        outColour = fragmentColour * texel;
    }
}
