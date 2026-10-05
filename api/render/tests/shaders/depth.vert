#version 450

/**
 * The vertex stage of a pipeline that writes depth and no colour, as a shadow pass does.
 * There is no fragment stage: the fixed function depth test writes depth, and a pipeline with
 * no colour attachment needs nothing else.
 *
 * Positions arrive in clip space, because the case compiles this to test the attachment count,
 * not a transform.
 **/

layout(location = 0) in vec3 position;

void main() {
    gl_Position = vec4(position, 1.0);
}
