#version 450

/**
 * The depth stage with every position put on the near plane. It reads the same input as
 * depth.vert and differs only in its code, so two pipelines built from the two stages differ
 * only in their vertex stage.
 **/

layout(location = 0) in vec3 position;

void main() {
    gl_Position = vec4(position.xy, 0.0, 1.0);
}
