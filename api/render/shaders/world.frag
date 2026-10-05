#version 450

/**
 * The fragment stage of the world space quad primitive.
 *
 * quad.frag without the text branch. There is one sampler and it is always bound, because an
 * untextured quad is drawn against a 1x1 white texture rather than through a second
 * pipeline. Nothing draws distance field glyphs in world space, so there is nothing here to
 * threshold.
 **/

layout(location = 0) in vec2 fragmentUv;
layout(location = 1) in vec4 fragmentColour;

layout(location = 0) out vec4 outColour;

// set 1 holds per material data
layout(set = 1, binding = 0) uniform sampler2D albedo;

void main() {
    outColour = fragmentColour * texture(albedo, fragmentUv);
}
