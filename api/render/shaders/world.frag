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
 *
 * Text's coverage is corrected by text.glsl, as quad.frag's is. Built with ENCODED_TARGET, for
 * an _SRGB target, a quad's colour is in linear light.
 **/

layout(location = 0) in vec2 fragmentUv;
layout(location = 1) in vec4 fragmentColour;
layout(location = 2) flat in uint fragmentGlyph;
layout(location = 3) flat in float fragmentContrast;

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

#ifdef ENCODED_TARGET
// in an _SRGB target a colour is in linear light, and its displayed value is its encoding
#include "srgb.glsl"
#endif

#include "text.glsl"

// the colour as it is displayed, for text's correction
vec3 shown(vec3 colour) {
#ifdef ENCODED_TARGET
    return display(colour);
#else
    return colour;
#endif
}

#include "colour.glsl"

void main() {
    // the em square's size in pixels, taken here, where every sample of a quad reaches it
    vec2 pixelsPerEm = 1.0 / max(fwidth(fragmentUv), vec2(1.0 / 65536.0));
    if (push.mode == CURVES && (fragmentGlyph & COLOUR_PROGRAM) != 0u) {
        // a colour glyph's colours are as they are displayed, and so is the text's colour it may
        // take. An _SRGB target is given them in linear light
        vec4 painted = colourGlyph(push.texture, push.bands, fragmentUv, fragmentGlyph & ~COLOUR_PROGRAM, vec4(shown(fragmentColour.rgb), 1.0), pixelsPerEm);
        outColour = painted.a > 0.0 ? vec4(painted.rgb / painted.a, painted.a * fragmentColour.a) : vec4(0.0);
#ifdef ENCODED_TARGET
        outColour.rgb = linear(outColour.rgb);
#endif
        return;
    }
    if (push.mode == CURVES) {
        // uv is the sample's place in the glyph, in em units
        float coverage = textCoverage(curveCoverage(push.texture, push.bands, fragmentUv, fragmentGlyph), shown(fragmentColour.rgb), fragmentContrast);
        outColour = vec4(fragmentColour.rgb, fragmentColour.a * coverage);
        return;
    }
    vec4 texel = texture(textures[push.texture], fragmentUv);
    if (push.mode == DISTANCE) {
        float coverage = textCoverage(fieldCoverage(texel.a, fragmentGlyph), shown(fragmentColour.rgb), fragmentContrast);
        outColour = vec4(fragmentColour.rgb, fragmentColour.a * coverage);
        return;
    }
    outColour = fragmentColour * texel;
}
