#version 450
#extension GL_EXT_nonuniform_qualifier : require

/**
 * The fragment stage of the world space quad primitive.
 *
 * quad.frag without the text branch. Every texture is read from the one array at set 1, at the
 * slot the batch pushes, and an untextured quad reads the 1x1 white texture's slot rather than
 * going through a second pipeline. Nothing draws distance field glyphs in world space, so there
 * is nothing here to threshold.
 **/

layout(location = 0) in vec2 fragmentUv;
layout(location = 1) in vec4 fragmentColour;

layout(location = 0) out vec4 outColour;

// set 1 is every texture the context holds, and the batch's is the one at its slot
layout(set = 1, binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform Push {
    vec4 tint;
    uint texture;
} push;

void main() {
    outColour = fragmentColour * texture(textures[push.texture], fragmentUv);
}
