#version 450

/**
 * The vertex stage of the world space quad primitive.
 *
 * The same quad as quad.vert's with a world position rather than a pixel one. The camera at
 * set 0 is the only transform, as it is for a line.
 **/

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 colour;

layout(location = 0) out vec2 fragmentUv;
layout(location = 1) out vec4 fragmentColour;

// set 0 holds per frame data: the camera the whole pass draws through
layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 viewport;
} camera;

void main() {
    gl_Position = camera.viewProjection * vec4(position, 1.0);
    fragmentUv = uv;
    fragmentColour = colour;
}
