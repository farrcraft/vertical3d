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
 *
 * A quad's colour is display-referred: the value written is the value that should appear. An
 * _SRGB target encodes what it stores, so it would show that value a shade too bright. Built
 * with LINEARISE, the stage decodes the colour to linear before writing it, and the target's
 * encoding gives the authored value back.
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

#ifdef LINEARISE
// the sRGB decode, per channel, which the target's encode on store undoes
vec3 linear(vec3 display) {
    vec3 low = display / 12.92;
    vec3 high = pow((display + 0.055) / 1.055, vec3(2.4));
    return mix(high, low, lessThanEqual(display, vec3(0.04045)));
}
#endif

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
#ifdef LINEARISE
    outColour.rgb = linear(outColour.rgb);
#endif
}
