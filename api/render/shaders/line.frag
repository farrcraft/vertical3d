#version 450

/**
 * The fragment stage of the world space line primitive.
 *
 * A segment's colour is interpolated from its two ends and written out. There is no texture
 * and no material, so the line pipeline declares set 0 and nothing else.
 **/

layout(location = 0) in vec4 fragmentColour;

layout(location = 0) out vec4 outColour;

void main() {
    outColour = fragmentColour;
}
