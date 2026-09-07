#version 450

/**
 * The vertex half of the one batched quad primitive - ADR-0005.
 *
 * Every 2D thing the engine draws goes through this: a coloured rectangle, a sprite, and a
 * glyph are the same quad with a different texture bound. Positions arrive in pixels and the
 * projection maps them to clip space, so nothing on the CPU side has to know about vulkan's
 * coordinate conventions.
 **/

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 colour;

layout(location = 0) out vec2 fragmentUv;
layout(location = 1) out vec4 fragmentColour;

// per object, per the binding convention in docs/RenderingPipeline.md. text is the
// fragment stage's, and is declared here because the range is one and covers both
layout(push_constant) uniform Push {
    mat4 projection;
    uint text;
} push;

void main() {
    gl_Position = push.projection * vec4(position, 0.0, 1.0);
    fragmentUv = uv;
    fragmentColour = colour;
}
