#version 450

/**
 * The fragment half of the line primitive - ADR-0011.
 *
 * A segment's colour is interpolated from its two ends and written out. There is no texture
 * and no material, which is why the line pipeline declares set 0 and nothing else.
 **/

layout(location = 0) in vec4 fragmentColour;

layout(location = 0) out vec4 outColour;

void main() {
    outColour = fragmentColour;
}
