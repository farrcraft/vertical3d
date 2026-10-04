#version 450

/**
 * The colour grade: the scene, looked up in a 16 cubed table.
 *
 * Both sides are linear light. A lit scene is drawn into an sRGB target, which decodes when it
 * is sampled (ADR-0066), so the table is indexed by linear colour, and what it returns is
 * written to a target that encodes again on store.
 **/

layout(set = 1, binding = 0) uniform sampler2D scene;
layout(set = 1, binding = 1) uniform sampler3D table;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 colour;

// a colour of 0 or 1 would land on the edge of the outer texels rather than their centres, and
// be filtered half with the clamp. Scaling into the centres keeps the ends exact
const float size = 16.0;
const float scale = (size - 1.0) / size;
const float bias = 0.5 / size;

void main() {
    const vec3 lit = texture(scene, uv).rgb;
    colour = vec4(texture(table, lit * scale + bias).rgb, 1.0);
}
