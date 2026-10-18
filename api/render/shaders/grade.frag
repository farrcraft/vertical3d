#version 450
#extension GL_GOOGLE_include_directive : require

/**
 * The colour grade: the scene, looked up in a 16 cubed table.
 *
 * The table maps a colour as it is shown to the colour it is shown as, as a strip authored in an
 * image editor does. A scene read from an sRGB target arrives decoded to linear light, so a
 * stage built with LINEAR_SCENE encodes it before the lookup. An sRGB target encodes what it
 * stores, so a stage built with LINEAR_TARGET decodes the result before writing it.
 **/

#include "srgb.glsl"

layout(set = 1, binding = 0) uniform sampler2D scene;
layout(set = 1, binding = 1) uniform sampler3D table;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 colour;

// a colour of 0 or 1 would land on the edge of the outer texels rather than their centres, and
// be half filtered with the clamp. Scaling into the texel centres keeps the ends exact
const float size = 16.0;
const float scale = (size - 1.0) / size;
const float bias = 0.5 / size;

void main() {
    vec3 shown = texture(scene, uv).rgb;
#ifdef LINEAR_SCENE
    shown = display(shown);
#endif
    vec3 graded = texture(table, shown * scale + bias).rgb;
#ifdef LINEAR_TARGET
    graded = linear(graded);
#endif
    colour = vec4(graded, 1.0);
}
