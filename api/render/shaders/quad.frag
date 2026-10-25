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
 * The only branch is text. A glyph atlas holds signed distances rather than coverage, and a
 * distance has to be thresholded to become an alpha. The branch is uniform across a draw,
 * because a batch is either all text or none.
 *
 * A quad's colour is display-referred: the value written is the value that should appear. An
 * _SRGB target encodes what it stores, so it would show that value a shade too bright. Built
 * with LINEARISE, the stage decodes the colour to linear before writing it, and the target's
 * encoding gives the authored value back.
 **/

layout(location = 0) in vec2 fragmentUv;
layout(location = 1) in vec4 fragmentColour;

layout(location = 0) out vec4 outColour;

// set 1 is every texture the context holds, and the batch's is the one at its slot
layout(set = 1, binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform Push {
    mat4 projection;
    uint text;
    uint texture;
} push;

#ifdef LINEARISE
// the decode, which the target's encode on store undoes
#include "srgb.glsl"
#endif

void main() {
    vec4 texel = texture(textures[push.texture], fragmentUv);

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
#ifdef LINEARISE
    outColour.rgb = linear(outColour.rgb);
#endif
}
