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
 * The branches are how a batch is drawn, uniform across a draw. A distance field holds signed
 * distances rather than coverage, and a distance has to be thresholded to become an alpha. The
 * quad's glyph field says where: at the glyph's edge, or where an effect drawn from the field
 * reaches. A curve glyph is drawn from its quadratic curves, which say how much of each pixel
 * it covers at any size and under any transform. Everything else multiplies its texel into
 * its colour, but for a coverage glyph, whose texel's alpha is its coverage.
 *
 * Text's coverage is corrected by text.glsl, so it looks the same weight in a UNORM and an
 * _SRGB target. A quad whose contrast is below 0 is not text.
 *
 * A quad's colour is display-referred: the value written is the value that should appear. An
 * _SRGB target encodes what it stores, so it would show that value a shade too bright. Built
 * with LINEARISE, the stage decodes the colour to linear before writing it, and the target's
 * encoding gives the authored value back.
 **/

layout(location = 0) in vec2 fragmentUv;
layout(location = 1) in vec4 fragmentColour;
layout(location = 2) flat in uint fragmentGlyph;
layout(location = 3) flat in float fragmentContrast;

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
#include "field.glsl"

#ifdef LINEARISE
// the decode, which the target's encode on store undoes
#include "srgb.glsl"
#define ENCODED_TARGET
#endif

#include "text.glsl"
#include "colour.glsl"

void main() {
    // the em square's size in pixels, taken here, where every sample of a quad reaches it
    vec2 pixelsPerEm = 1.0 / max(fwidth(fragmentUv), vec2(1.0 / 65536.0));
    if (push.text == CURVES && (fragmentGlyph & COLOUR_PROGRAM) != 0u) {
        // a colour glyph in its own colours, the text's colour for a layer that takes it, faded
        // by the text's alpha
        vec4 painted = colourGlyph(push.texture, push.bands, fragmentUv, fragmentGlyph & ~COLOUR_PROGRAM, vec4(fragmentColour.rgb, 1.0), pixelsPerEm);
        outColour = painted.a > 0.0 ? vec4(painted.rgb / painted.a, painted.a * fragmentColour.a) : vec4(0.0);
#ifdef LINEARISE
        outColour.rgb = linear(outColour.rgb);
#endif
        return;
    }
    if (push.text == CURVES) {
        // uv is the sample's place in the glyph, in em units
        float coverage = textCoverage(curveCoverage(push.texture, push.bands, fragmentUv, fragmentGlyph), fragmentColour.rgb, fragmentContrast);
        outColour = vec4(fragmentColour.rgb, fragmentColour.a * coverage);
#ifdef LINEARISE
        outColour.rgb = linear(outColour.rgb);
#endif
        return;
    }

    vec4 texel = texture(textures[push.texture], fragmentUv);

    if (push.text == DISTANCE) {
        // a glyph's own edge, or an effect drawn from its field, by the quad's ramp
        float coverage = textCoverage(fieldCoverage(texel.a, fragmentGlyph), fragmentColour.rgb, fragmentContrast);
        outColour = vec4(fragmentColour.rgb, fragmentColour.a * coverage);
    } else if (fragmentContrast >= 0.0) {
        // a coverage glyph, its coverage in alpha
        outColour = vec4(fragmentColour.rgb, fragmentColour.a * textCoverage(texel.a, fragmentColour.rgb, fragmentContrast));
    } else {
        outColour = fragmentColour * texel;
    }
#ifdef LINEARISE
    outColour.rgb = linear(outColour.rgb);
#endif
}
