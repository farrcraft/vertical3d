#version 450
#extension GL_GOOGLE_include_directive : require

/**
 * The shadow pass: a caster drawn through the light rather than the camera, into one layer of
 * a target with depth and no colour. There is no fragment stage, because only depth is written.
 * A posed model casts the shadow of its current pose rather than its bind pose.
 *
 * Each cascade is drawn by a pass of its own, and each draw pushes the cascade it is drawn
 * into, which picks the matrix its layer is drawn through.
 **/

#include "lit.glsl"
#include "pose.glsl"

layout(push_constant) uniform Cascade {
    uint index;
} cascade;

layout(location = 0) in vec3 position;

void main() {
    gl_Position = scene.lightViewProjection[cascade.index] * object.model * pose() * vec4(position, 1.0);
}
