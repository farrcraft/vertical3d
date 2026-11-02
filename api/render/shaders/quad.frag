#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

/**
 * The fragment stage of the batched quad primitive.
 *
 * Every texture is read from the one array at set 1, at the slot the batch pushes. An
 * untextured quad reads the 1x1 white texture's slot rather than going through a second
 * pipeline. A single channel texture, such as a glyph atlas, is given a view that swizzles its
 * one channel into alpha, so the value is in alpha whatever the slot holds.
 *
 * The branches are how a batch is drawn, uniform across a draw. A distance field glyph holds
 * signed distances rather than coverage, and a distance has to be thresholded to become an
 * alpha. A curve glyph is drawn from its quadratic curves, which say how much of each pixel
 * it covers at any size and under any transform. Everything else multiplies its texel into
 * its colour.
 *
 * A quad's colour is display-referred: the value written is the value that should appear. An
 * _SRGB target encodes what it stores, so it would show that value a shade too bright. Built
 * with LINEARISE, the stage decodes the colour to linear before writing it, and the target's
 * encoding gives the authored value back.
 **/

layout(location = 0) in vec2 fragmentUv;
layout(location = 1) in vec4 fragmentColour;
layout(location = 2) flat in uint fragmentGlyph;

layout(location = 0) out vec4 outColour;

// set 1 is every texture the context holds, and the batch's is the one at its slot
layout(set = 1, binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform Push {
    mat4 projection;
    uint text;
    uint texture;
    uint bands;
} push;

// how a batch is drawn: a picture, a distance field, or glyphs found from their curves
const uint PICTURE = 0u;
const uint DISTANCE = 1u;
const uint CURVES = 2u;

#include "curves.glsl"

#ifdef LINEARISE
// the decode, which the target's encode on store undoes
#include "srgb.glsl"
#endif

void main() {
    if (push.text == CURVES) {
        // uv is the sample's place in the glyph, in em units
        float coverage = curveCoverage(push.texture, push.bands, fragmentUv, fragmentGlyph);
        outColour = vec4(fragmentColour.rgb, fragmentColour.a * coverage);
#ifdef LINEARISE
        outColour.rgb = linear(outColour.rgb);
#endif
        return;
    }

    vec4 texel = texture(textures[push.texture], fragmentUv);

    if (push.text == DISTANCE) {
        // the glyph's edge is where the distance crosses the midpoint the field was built
        // around. Coverage ramps from 0 to 1 across one pixel centred on it. A pixel's width
        // in distance is the length of the distance's screen gradient, so the ramp stays one
        // pixel wide at any scale and on a diagonal edge
        float distance = texel.a;
        float width = max(length(vec2(dFdx(distance), dFdy(distance))), 1e-5);
        float coverage = clamp(((distance - 0.5) / width) + 0.5, 0.0, 1.0);
        outColour = vec4(fragmentColour.rgb, fragmentColour.a * coverage);
    } else {
        outColour = fragmentColour * texel;
    }
#ifdef LINEARISE
    outColour.rgb = linear(outColour.rgb);
#endif
}
