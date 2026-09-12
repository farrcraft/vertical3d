#version 450

/**
 * The vertex half of a pipeline that writes depth and no colour, which is what a shadow
 * pass is. There is no fragment stage at all: depth is written by the fixed function
 * tests, so a pipeline with no colour attachment has nothing for one to do.
 *
 * Positions arrive in clip space, since what this exists to compile is the attachment
 * count rather than a transform.
 **/

layout(location = 0) in vec3 position;

void main() {
    gl_Position = vec4(position, 1.0);
}
