#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

/**
 * The fragment stage of the world space quad primitive.
 *
 * quad.frag without the distance field branch. Every texture is read from the one array at
 * set 1, at the slot the batch pushes, and an untextured quad reads the 1x1 white texture's
 * slot rather than going through a second pipeline. A batch of curve glyphs finds each pixel's
 * coverage from the curves that cross it, so text in the world is sharp at any distance and
 * angle. The branch is uniform across a draw.
 **/

layout(location = 0) in vec2 fragmentUv;
layout(location = 1) in vec4 fragmentColour;
layout(location = 2) flat in uint fragmentGlyph;

layout(location = 0) out vec4 outColour;

// set 1 is every texture the context holds, and the batch's is the one at its slot
layout(set = 1, binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform Push {
    vec4 tint;
    uint texture;
    uint bands;
    uint curves;
} push;

#include "curves.glsl"

void main() {
    if (push.curves != 0u) {
        // uv is the sample's place in the glyph, in em units
        float coverage = curveCoverage(push.texture, push.bands, fragmentUv, fragmentGlyph);
        outColour = vec4(fragmentColour.rgb, fragmentColour.a * coverage);
        return;
    }
    outColour = fragmentColour * texture(textures[push.texture], fragmentUv);
}
