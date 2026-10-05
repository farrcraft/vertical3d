#version 450
#extension GL_GOOGLE_include_directive : require

/**
 * The shadow pass: a caster drawn through the light rather than the camera, into a target
 * with depth and no colour. There is no fragment stage, because the depth test writes all
 * there is to write. A posed model casts its pose rather than the one it was bound in.
 **/

#include "lit.glsl"
#include "pose.glsl"

layout(location = 0) in vec3 position;

void main() {
    gl_Position = scene.lightViewProjection * object.model * pose() * vec4(position, 1.0);
}
