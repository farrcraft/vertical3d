#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

/**
 * The fragment stage of the world space quad primitive.
 *
 * quad.frag in the world. Every texture is read from the one array at set 1, at the slot the
 * batch pushes, and an untextured quad reads the 1x1 white texture's slot rather than going
 * through a second pipeline. A batch of curve glyphs finds each pixel's coverage from the
 * curves that cross it, so text in the world is sharp at any distance and angle. A batch of
 * distance fields draws the effects beneath it, each where the quad's ramp says. The branch is
 * uniform across a draw.
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
    uint mode;
} push;

// how a batch is drawn: a picture, glyphs found from their curves, or a distance field
const uint PICTURE = 0u;
const uint CURVES = 1u;
const uint DISTANCE = 2u;

#include "curves.glsl"
#include "field.glsl"

void main() {
    if (push.mode == CURVES) {
        // uv is the sample's place in the glyph, in em units
        float coverage = curveCoverage(push.texture, push.bands, fragmentUv, fragmentGlyph);
        outColour = vec4(fragmentColour.rgb, fragmentColour.a * coverage);
        return;
    }
    vec4 texel = texture(textures[push.texture], fragmentUv);
    if (push.mode == DISTANCE) {
        outColour = vec4(fragmentColour.rgb, fragmentColour.a * fieldCoverage(texel.a, fragmentGlyph));
        return;
    }
    outColour = fragmentColour * texel;
}
