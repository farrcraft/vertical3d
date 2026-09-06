#version 450

/**
 * The vertex half of the line primitive - ADR-0011.
 *
 * Positions arrive in world space and the camera the pass carries at set 0 is the whole of
 * the transform, so there is no push constant.
 **/

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 colour;

layout(location = 0) out vec4 fragmentColour;

// set 0, the per frame frequency of ADR-0008 - the camera the whole pass draws through
layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 viewport;
} camera;

void main() {
    gl_Position = camera.viewProjection * vec4(position, 1.0);
    fragmentColour = colour;
}
